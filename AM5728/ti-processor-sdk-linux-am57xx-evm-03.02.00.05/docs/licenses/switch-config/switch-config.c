/*
 * Ethernet Switch configuration management
 *
 * Copyright (C) 2014 Texas Instruments
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define true 1
#define false 0
typedef int bool;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <linux/net_switch_config.h>

typedef unsigned long long u64;
typedef __uint32_t u32;
typedef __uint16_t u16;
typedef __uint8_t u8;
typedef __int32_t s32;

enum {
	EXTENDED_CONFIG_SWITCH_INVALID,
	EXTENDED_CONFIG_SWITCH_DUMP,
};

#ifndef DIV_ROUND_UP
#define	DIV_ROUND_UP(x,y)	(((x) + ((y) - 1)) / (y))
#endif

#define CPSW_MAJOR_VERSION(reg)		(reg >> 8 & 0x7)
#define CPSW_MINOR_VERSION(reg)		(reg & 0xff)
#define CPSW_RTL_VERSION(reg)		((reg >> 11) & 0x1f)

#define ADDR_FMT_ARGS(addr)	(addr)[0], (addr)[1], (addr)[2], \
				(addr)[3], (addr)[4], (addr)[5]

#define ALE_ENTRY_BITS          68
#define ALE_ENTRY_WORDS         DIV_ROUND_UP(ALE_ENTRY_BITS, 32)

#define BIT(nr)			(1 << (nr))
#define BITMASK(bits)		(BIT(bits) - 1)

#define ALE_TYPE_FREE			0
#define ALE_TYPE_ADDR			1
#define ALE_TYPE_VLAN			2
#define ALE_TYPE_VLAN_ADDR		3

static inline int cpsw_ale_get_field(u32 *ale_entry, u32 start, u32 bits)
{
	int idx;

	idx    = start / 32;
	start -= idx * 32;
	idx    = 2 - idx; /* flip */
	return (ale_entry[idx] >> start) & BITMASK(bits);
}

static inline void cpsw_ale_set_field(u32 *ale_entry, u32 start, u32 bits,
				      u32 value)
{
	int idx;

	value &= BITMASK(bits);
	idx    = start / 32;
	start -= idx * 32;
	idx    = 2 - idx; /* flip */
	ale_entry[idx] &= ~(BITMASK(bits) << start);
	ale_entry[idx] |=  (value << start);
}

#define DEFINE_ALE_FIELD(name, start, bits)				\
static inline int cpsw_ale_get_##name(u32 *ale_entry)			\
{									\
	return cpsw_ale_get_field(ale_entry, start, bits);		\
}									\
static inline void cpsw_ale_set_##name(u32 *ale_entry, u32 value)	\
{									\
	cpsw_ale_set_field(ale_entry, start, bits, value);		\
}

DEFINE_ALE_FIELD(entry_type,		60,	2)
DEFINE_ALE_FIELD(vlan_id,		48,	12)
DEFINE_ALE_FIELD(mcast_state,		62,	2)
DEFINE_ALE_FIELD(port_mask,		66,     3)
DEFINE_ALE_FIELD(super,			65,	1)
DEFINE_ALE_FIELD(ucast_type,		62,     2)
DEFINE_ALE_FIELD(port_num,		66,     2)
DEFINE_ALE_FIELD(blocked,		65,     1)
DEFINE_ALE_FIELD(secure,		64,     1)
DEFINE_ALE_FIELD(vlan_untag_force,	24,	3)
DEFINE_ALE_FIELD(vlan_reg_mcast,	16,	3)
DEFINE_ALE_FIELD(vlan_unreg_mcast,	8,	3)
DEFINE_ALE_FIELD(vlan_member_list,	0,	3)
DEFINE_ALE_FIELD(mcast,			40,	1)

static inline void cpsw_ale_get_addr(u32 *ale_entry, u8 *addr)
{
	int i;

	for (i = 0; i < 6; i++)
		addr[i] = cpsw_ale_get_field(ale_entry, 40 - 8*i, 8);
}

static void cpsw_ale_dump_vlan(int index, u32 *ale_entry)
{
	int vlan = cpsw_ale_get_vlan_id(ale_entry);
	int untag_force = cpsw_ale_get_vlan_untag_force(ale_entry);
	int reg_mcast   = cpsw_ale_get_vlan_reg_mcast(ale_entry);
	int unreg_mcast = cpsw_ale_get_vlan_unreg_mcast(ale_entry);
	int member_list = cpsw_ale_get_vlan_member_list(ale_entry);

	fprintf(stdout, "%-4d: type: vlan , vid = %d, untag_force = 0x%x, reg_mcast = 0x%x, unreg_mcast = 0x%x, member_list = 0x%x\n",
		index, vlan, untag_force, reg_mcast, unreg_mcast, member_list);
}

static void cpsw_ale_dump_addr(int index, u32 *ale_entry)
{
	u8 addr[6];

	cpsw_ale_get_addr(ale_entry, addr);
	if (cpsw_ale_get_mcast(ale_entry)) {
		static const char *str_mcast_state[] = {"f", "blf", "lf", "f"};
		int state     = cpsw_ale_get_mcast_state(ale_entry);
		int port_mask = cpsw_ale_get_port_mask(ale_entry);
		int super     = cpsw_ale_get_super(ale_entry);

		fprintf(stdout, "%-4d: type: mcast, addr = %02x:%02x:%02x:%02x:%02x:%02x, mcast_state = %s, %ssuper, port_mask = 0x%x\n",
			index, ADDR_FMT_ARGS(addr), str_mcast_state[state],
			super ? "" : "no ", port_mask);
	} else {
		static const char *s_ucast_type[] = {"persistant", "untouched ",
						     "oui       ", "touched   "};
		int ucast_type = cpsw_ale_get_ucast_type(ale_entry);
		int port_num   = cpsw_ale_get_port_num(ale_entry);
		int secure     = cpsw_ale_get_secure(ale_entry);
		int blocked    = cpsw_ale_get_blocked(ale_entry);

		fprintf(stdout, "%-4d: type: ucast, addr = %02x:%02x:%02x:%02x:%02x:%02x, ucast_type = %s, port_num = 0x%x%s%s\n",
			index, ADDR_FMT_ARGS(addr), s_ucast_type[ucast_type],
			port_num, secure ? ", Secure" : "",
			blocked ? ", Blocked" : "");
	}
}

static void cpsw_ale_dump_vlan_addr(int index, u32 *ale_entry)
{
	u8 addr[6];
	int vlan = cpsw_ale_get_vlan_id(ale_entry);

	cpsw_ale_get_addr(ale_entry, addr);
	if (cpsw_ale_get_mcast(ale_entry)) {
		static const char *str_mcast_state[] = {"f", "blf", "lf", "f"};
		int state     = cpsw_ale_get_mcast_state(ale_entry);
		int port_mask = cpsw_ale_get_port_mask(ale_entry);
		int super     = cpsw_ale_get_super(ale_entry);

		fprintf(stdout, "%-4d: type: mcast, vid = %d, addr = %02x:%02x:%02x:%02x:%02x:%02x, mcast_state = %s, %ssuper, port_mask = 0x%x\n",
			index, vlan, ADDR_FMT_ARGS(addr),
			str_mcast_state[state], super ? "" : "no ", port_mask);
	} else {
		static const char *s_ucast_type[] = {"persistant", "untouched ",
						     "oui       ", "touched   "};
		int ucast_type = cpsw_ale_get_ucast_type(ale_entry);
		int port_num   = cpsw_ale_get_port_num(ale_entry);
		int secure     = cpsw_ale_get_secure(ale_entry);
		int blocked    = cpsw_ale_get_blocked(ale_entry);

		fprintf(stdout, "%-4d: type: ucast, vid = %d, addr = %02x:%02x:%02x:%02x:%02x:%02x, ucast_type = %s, port_num = 0x%x%s%s\n",
			index, vlan, ADDR_FMT_ARGS(addr),
			s_ucast_type[ucast_type], port_num,
			secure ? ", Secure" : "", blocked ? ", Blocked" : "");
	}
}

int cpsw_dump_regs(struct ethtool_drvinfo *info, struct ethtool_regs *regs)
{
	u32 *reg = (u32 *)regs->data;
	int i;

	fprintf(stdout, "cpsw hw version %d.%d (%d)\n",
		CPSW_MAJOR_VERSION(regs->version),
		CPSW_MINOR_VERSION(regs->version),
		CPSW_RTL_VERSION(regs->version));

	for(i = 0; i < 1024; i++, reg += ALE_ENTRY_WORDS) {
		int type;

		type = cpsw_ale_get_entry_type(reg);
		switch (type) {
		case ALE_TYPE_FREE:
			break;

		case ALE_TYPE_ADDR:
			cpsw_ale_dump_addr(i, reg);
			break;

		case ALE_TYPE_VLAN:
			cpsw_ale_dump_vlan(i, reg);
			break;

		case ALE_TYPE_VLAN_ADDR:
			cpsw_ale_dump_vlan_addr(i, reg);
			break;

		default:
			fprintf(stdout, "%-4d: Invalid Entry type\n", i);
		}
	}

	return 0;
}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define SWITCH_CONFIG_COMMAND(__var__, __cmd__)			\
	if((__var__) == CONFIG_SWITCH_INVALID) {		\
		(__var__) = (__cmd__);				\
	} else {						\
		printf("Two or more commands Cannot be "	\
			"processed simultaneously\n");		\
		return -1;					\
	}

static char options[] = "?m:x:i:y:S:GUg:udn:V:sk:K:M:N:D:Y:z:Z:J:vPp:r:ClB:L:t";

static struct option long_options[] =
	{
		/* These options set a flag. */
{"add-multi",			required_argument	, 0, 'm'},
{"del-multi",			required_argument	, 0, 'x'},
{"add-vlan",			required_argument	, 0, 'i'},
{"del-vlan",			required_argument	, 0, 'y'},
{"add-unknown-vlan-info",	no_argument		, 0, 'U'},
{"set-port-config",		required_argument	, 0, 'S'},
{"get-port-config",		no_argument		, 0, 'G'},
{"set-port-state",		required_argument	, 0, 'g'},
{"get-port-state",		no_argument		, 0, 'u'},
{"dump",			optional_argument	, 0, 'd'},
{"port",			required_argument	, 0, 'n'},
{"vid",				required_argument	, 0, 'V'},
{"super",			no_argument		, 0, 's'},
{"fwd-state",			required_argument	, 0, 'k'},
{"vid-untag",			required_argument	, 0, 'K'},
{"reg-multi",			required_argument	, 0, 'M'},
{"unreg-multi",			required_argument	, 0, 'N'},
{"vid-untag-port-mask",		required_argument	, 0, 'Y'},
{"reg-multi-port-mask",		required_argument	, 0, 'z'},
{"unreg-multi-port-mask",	required_argument	, 0, 'Z'},
{"unknown-vlan-mem",		required_argument	, 0, 'J'},
{"duplex",			required_argument	, 0, 'D'},
{"version",			no_argument		, 0, 'v'},
{"get-port-vlan",		no_argument		, 0, 'P'},
{"set-port-vlan",		required_argument	, 0, 'p'},
{"priority",			required_argument	, 0, 'r'},
{"cfi",				no_argument		, 0, 'C'},
{"rate-limit",			no_argument		, 0, 'l'},
{"bcast-limit",			required_argument	, 0, 'B'},
{"mcast-limit",			required_argument	, 0, 'L'},
{"direction",			no_argument		, 0, 't'},
{0, 0, 0, 0}
};

static inline int is_zero_ether_addr(const unsigned char *addr)
{
	return !(addr[0] | addr[1] | addr[2] | addr[3] | addr[4] | addr[5]);
}

static inline int is_multicast_ether_addr(const unsigned char *addr)
{
	return 0x01 & addr[0];
}

static inline int is_valid_ether_addr(const unsigned char *addr)
{
	return !is_multicast_ether_addr(addr) && !is_zero_ether_addr(addr);
}

/* String to Hex conversion */
static unsigned char cpmac_str_to_hexnum(unsigned char c)
{
	if(c >= '0' && c <= '9') return c - '0';
	if(c >= 'a' && c <= 'f') return c - 'a' + 10;
	if(c >= 'A' && c <= 'F') return c - 'A' + 10;
	return 0;
}

/* String to ethernet address conversion */
static void config_switch_str_to_ethaddr(char *str, unsigned char *addr)
{
	int i;
	unsigned char num;
	for(i = 0; i < 6; i++) {
		if((*str == '.') || (*str == ':')) str++;
		num = cpmac_str_to_hexnum(*str++) << 4;
		num |= (cpmac_str_to_hexnum(*str++));
		addr[i] = num;
	}
}

static void
dump_link_caps(const char *prefix, const char *an_prefix, u32 mask,
	       int link_mode_only)
{
	static const struct {
		int same_line; /* print on same line as previous */
		u32 value;
		const char *name;
	} mode_defs[] = {
		{ 0, ADVERTISED_10baseT_Half,       "10baseT/Half" },
		{ 1, ADVERTISED_10baseT_Full,       "10baseT/Full" },
		{ 0, ADVERTISED_100baseT_Half,      "100baseT/Half" },
		{ 1, ADVERTISED_100baseT_Full,      "100baseT/Full" },
		{ 0, ADVERTISED_1000baseT_Half,     "1000baseT/Half" },
		{ 1, ADVERTISED_1000baseT_Full,     "1000baseT/Full" },
		{ 0, ADVERTISED_1000baseKX_Full,    "1000baseKX/Full" },
		{ 0, ADVERTISED_2500baseX_Full,     "2500baseX/Full" },
		{ 0, ADVERTISED_10000baseT_Full,    "10000baseT/Full" },
		{ 0, ADVERTISED_10000baseKX4_Full,  "10000baseKX4/Full" },
		{ 0, ADVERTISED_10000baseKR_Full,   "10000baseKR/Full" },
		{ 0, ADVERTISED_20000baseMLD2_Full, "20000baseMLD2/Full" },
		{ 0, ADVERTISED_20000baseKR2_Full,  "20000baseKR2/Full" },
		{ 0, ADVERTISED_40000baseKR4_Full,  "40000baseKR4/Full" },
		{ 0, ADVERTISED_40000baseCR4_Full,  "40000baseCR4/Full" },
		{ 0, ADVERTISED_40000baseSR4_Full,  "40000baseSR4/Full" },
		{ 0, ADVERTISED_40000baseLR4_Full,  "40000baseLR4/Full" },
	};
	int indent;
	int did1, new_line_pend, i;

	/* Indent just like the separate functions used to */
	indent = strlen(prefix) + 14;
	if (indent < 24)
		indent = 24;

	fprintf(stdout, "	%s link modes:%*s", prefix,
		indent - (int)strlen(prefix) - 12, "");
	did1 = 0;
	new_line_pend = 0;
	for (i = 0; i < ARRAY_SIZE(mode_defs); i++) {
		if (did1 && !mode_defs[i].same_line)
			new_line_pend = 1;
		if (mask & mode_defs[i].value) {
			if (new_line_pend) {
				fprintf(stdout, "\n");
				fprintf(stdout, "	%*s", indent, "");
				new_line_pend = 0;
			}
			did1++;
			fprintf(stdout, "%s ", mode_defs[i].name);
		}
	}
	if (did1 == 0)
		 fprintf(stdout, "Not reported");
	fprintf(stdout, "\n");

	if (!link_mode_only) {
		fprintf(stdout, "	%s pause frame use: ", prefix);
		if (mask & ADVERTISED_Pause) {
			fprintf(stdout, "Symmetric");
			if (mask & ADVERTISED_Asym_Pause)
				fprintf(stdout, " Receive-only");
			fprintf(stdout, "\n");
		} else {
			if (mask & ADVERTISED_Asym_Pause)
				fprintf(stdout, "Transmit-only\n");
			else
				fprintf(stdout, "No\n");
		}

		fprintf(stdout, "	%s auto-negotiation: ", an_prefix);
		if (mask & ADVERTISED_Autoneg)
			fprintf(stdout, "Yes\n");
		else
			fprintf(stdout, "No\n");
	}
}

static int dump_ecmd(struct ethtool_cmd *ep)
{
	u32 speed;

	dump_link_caps("Advertised", "Advertised", ep->advertising, 0);
	if (ep->lp_advertising)
		dump_link_caps("Link partner advertised",
			       "Link partner advertised", ep->lp_advertising,
			       0);

	fprintf(stdout, "	Speed: ");
	speed = ethtool_cmd_speed(ep);
	if (speed == 0 || speed == (u16)(-1) || speed == (u32)(-1))
		fprintf(stdout, "Unknown!\n");
	else
		fprintf(stdout, "%uMb/s\n", speed);

	fprintf(stdout, "	Duplex: ");
	switch (ep->duplex) {
	case DUPLEX_HALF:
		fprintf(stdout, "Half\n");
		break;
	case DUPLEX_FULL:
		fprintf(stdout, "Full\n");
		break;
	default:
		fprintf(stdout, "Unknown! (%i)\n", ep->duplex);
		break;
	};

	fprintf(stdout, "	Port: ");
	switch (ep->port) {
	case PORT_TP:
		fprintf(stdout, "Twisted Pair\n");
		break;
	case PORT_AUI:
		fprintf(stdout, "AUI\n");
		break;
	case PORT_BNC:
		fprintf(stdout, "BNC\n");
		break;
	case PORT_MII:
		fprintf(stdout, "MII\n");
		break;
	case PORT_FIBRE:
		fprintf(stdout, "FIBRE\n");
		break;
	case PORT_DA:
		fprintf(stdout, "Direct Attach Copper\n");
		break;
	case PORT_NONE:
		fprintf(stdout, "None\n");
		break;
	case PORT_OTHER:
		fprintf(stdout, "Other\n");
		break;
	default:
		fprintf(stdout, "Unknown! (%i)\n", ep->port);
		break;
	};

	fprintf(stdout, "	PHYAD: %d\n", ep->phy_address);
	fprintf(stdout, "	Transceiver: ");
	switch (ep->transceiver) {
	case XCVR_INTERNAL:
		fprintf(stdout, "internal\n");
		break;
	case XCVR_EXTERNAL:
		fprintf(stdout, "external\n");
		break;
	default:
		fprintf(stdout, "Unknown!\n");
		break;
	};

	fprintf(stdout, "	Auto-negotiation: %s\n",
		(ep->autoneg == AUTONEG_DISABLE) ?
		"off" : "on");

	if (ep->port == PORT_TP) {
		fprintf(stdout, "	MDI-X: ");
		if (ep->eth_tp_mdix_ctrl == ETH_TP_MDI) {
			fprintf(stdout, "off (forced)\n");
		} else if (ep->eth_tp_mdix_ctrl == ETH_TP_MDI_X) {
			fprintf(stdout, "on (forced)\n");
		} else {
			switch (ep->eth_tp_mdix) {
			case ETH_TP_MDI:
				fprintf(stdout, "off");
				break;
			case ETH_TP_MDI_X:
				fprintf(stdout, "on");
				break;
			default:
				fprintf(stdout, "Unknown");
				break;
			}
			if (ep->eth_tp_mdix_ctrl == ETH_TP_MDI_AUTO)
				fprintf(stdout, " (auto)");
			fprintf(stdout, "\n");
		}
	}

	return 0;
}

void print_help(void)
{
	printf(
		"Switch configuration commands.....\n"
		"All input and output values are in Decimal\n"
		"switch-config -m,--add-multi <address> -n,--port <Portmask> "
			"[-V,--vid <vid>] [-s,--super] "
			"[-k,--fwd-state <value 0-3>]\n"
		"switch-config -x,--del-multi <address> [-V,--vid <vid>]\n"
		"switch-config -i,--add-vlan <vlan id> -n,--port <Portmask> "
			"[-K,--vid-untag <Portmask>] "
			"[-M,--reg-multi <Portmask>] "
			"[-N,--unreg-multi <Portmask>]\n"
		"switch-config -y,--del-vlan <vid>\n"
		"switch-config -S,--set-port-config <0(auto)/10/100/1000> "
			"-n,--port <PortNo> [-D,--duplex <full/half>]\n"
		"switch-config -G,--get-port-config -n,--port <PortNo>\n"
		"switch-config -U,--add-unknown-vlan-info "
			"[-Y,--vid-untag-port-mask <0-7>] [-z,--reg-multi-port-mask <0-7>] "
			"[-Z,--unreg-multi-port-mask <0-7>] "
			"[-J,--unknown-vlan-mem <0-7>]\n"
		"switch-config -g,--set-port-state <disabled/blocked/learn/forward> "
			"-n,--port <PortNo>\n"
		"switch-config -u,--get-port-state -n,--port <PortNo>\n"
		"switch-config -P,--get-port-vlan -n,--port <Port No 0-2>\n"
		"switch-config -p,--set-port-vlan <vid> -n,--port <Port No 0-2> "
			"[-r,--priority <priority 0-7>] [-C,--cfi]\n"
		"switch-config -l,--rate-limit -n,--port <Port No> "
			"-B,--bcast-limit <No of Packet 0-255> "
			"-L,--limit <No of Packet 0-255> "
			"[-t,--direction specify for Tx] ]\n"
		"switch-config -d,--dump\n"
		"switch-config -v,--version\n"
		"\n"
		);
}

int main(int argc, char **argv)
{
	int option_index = 0;
	int c;
	struct net_switch_config cmd_struct;
	unsigned int port_num = 0;
	struct ifreq ifr;
	int sockfd;  /* socket fd we use to manipulate stuff with */
	unsigned int speed;
	unsigned int duplex;
	unsigned int autoneg;
	int extended_switch_config;

	/* get interface name */
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
	ifr.ifr_data = (char*)&cmd_struct;
	/* Create a channel to the NET kernel. */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Can't open the socket\n");
		return -1;
	}
	memset(&cmd_struct, 0, sizeof(struct net_switch_config));

	/* parse command line arguments */
	while ((c = getopt_long(argc, argv, options, long_options,
			&option_index)) != -1) {
		switch (c) {
		case '?':
			print_help();
			return 0;
		case 'v':
			printf("switch-config version: %s\n", VERSION);
			return 0;

		case 'm':
			SWITCH_CONFIG_COMMAND(cmd_struct.cmd,
				CONFIG_SWITCH_ADD_MULTICAST);
			config_switch_str_to_ethaddr(optarg,
				cmd_struct.addr);
		break;

		case 'x':
			SWITCH_CONFIG_COMMAND(cmd_struct.cmd,
				CONFIG_SWITCH_DEL_MULTICAST);
			config_switch_str_to_ethaddr(optarg,
				cmd_struct.addr);
		break;

		case 'i':
			SWITCH_CONFIG_COMMAND(cmd_struct.cmd,
				CONFIG_SWITCH_ADD_VLAN);
			cmd_struct.vid = atoi(optarg);
		break;

		case 'y':
			SWITCH_CONFIG_COMMAND(cmd_struct.cmd,
				CONFIG_SWITCH_DEL_VLAN);
			cmd_struct.vid = atoi(optarg);
		break;

		case 'G':
			SWITCH_CONFIG_COMMAND(cmd_struct.cmd,
				CONFIG_SWITCH_GET_PORT_CONFIG);
		break;

		case 'S':
			SWITCH_CONFIG_COMMAND(cmd_struct.cmd,
				CONFIG_SWITCH_SET_PORT_CONFIG);
			speed = atoi(optarg);
			if (speed == 0)
				autoneg = AUTONEG_ENABLE;
			else
				autoneg = AUTONEG_DISABLE;
		break;

		case 'U':
			SWITCH_CONFIG_COMMAND(cmd_struct.cmd,
				CONFIG_SWITCH_ADD_UNKNOWN_VLAN_INFO);
		break;

		case 'g':
			SWITCH_CONFIG_COMMAND(cmd_struct.cmd,
				CONFIG_SWITCH_SET_PORT_STATE);
			if (!strcmp(optarg, "disabled"))
				cmd_struct.port_state = PORT_STATE_DISABLED;
			else if (!strcmp(optarg, "blocked"))
				cmd_struct.port_state = PORT_STATE_BLOCKED;
			else if (!strcmp(optarg, "learn"))
				cmd_struct.port_state = PORT_STATE_LEARN;
			else if (!strcmp(optarg, "forward"))
				cmd_struct.port_state = PORT_STATE_FORWARD;
			else {
				printf("Invalid Port State\n");
				return -1;
			}
		break;

		case 'u':
			SWITCH_CONFIG_COMMAND(cmd_struct.cmd,
				CONFIG_SWITCH_GET_PORT_STATE);
		break;

		case 'P':
			SWITCH_CONFIG_COMMAND(cmd_struct.cmd,
				CONFIG_SWITCH_GET_PORT_VLAN_CONFIG);
		break;

		case 'p':
			SWITCH_CONFIG_COMMAND(cmd_struct.cmd,
				CONFIG_SWITCH_SET_PORT_VLAN_CONFIG);
			cmd_struct.vid = atoi(optarg);
		break;

		case 'l':
			SWITCH_CONFIG_COMMAND(cmd_struct.cmd,
				CONFIG_SWITCH_RATELIMIT);
		break;

		/* Extended switch commands */
		case 'd':
			extended_switch_config = EXTENDED_CONFIG_SWITCH_DUMP;
		break;

/* Command arguments */
		case 'n':
			port_num = atoi(optarg);
		break;

		case 'V':
			cmd_struct.vid = atoi(optarg);
		break;

		case 'k':
			cmd_struct.untag_port = atoi(optarg);
		break;

		case 'K':
			cmd_struct.untag_port = atoi(optarg);
		break;

		case 'M':
			cmd_struct.reg_multi = atoi(optarg);
		break;

		case 'N':
			cmd_struct.unreg_multi = atoi(optarg);
		break;

		case 'D':
			if (!strcmp(optarg, "full"))
				duplex = DUPLEX_FULL;
			else if (!strcmp(optarg, "half"))
				duplex = DUPLEX_HALF;
			else {
				printf("Invalid Options for Duplex\n");
				close(sockfd);
				return -1;
			}
		break;

		case 'Y':
			cmd_struct.unknown_vlan_untag = atoi(optarg);
		break;

		case 'z':
			cmd_struct.unknown_vlan_reg_multi = atoi(optarg);
		break;

		case 'Z':
			cmd_struct.unknown_vlan_unreg_multi = atoi(optarg);
		break;

		case 'J':
			cmd_struct.unknown_vlan_member = atoi(optarg);
		break;

		case 'r':
			cmd_struct.prio = atoi(optarg);
		break;

		case 'C':
			cmd_struct.vlan_cfi = true;
		break;

		case 'B':
			cmd_struct.bcast_rate_limit = atoi(optarg);
		break;

		case 'L':
			cmd_struct.mcast_rate_limit = atoi(optarg);
		break;

		case 't':
			cmd_struct.direction = true;
		break;

		default:
			print_help();
			return 0;
		}
	}

	switch (cmd_struct.cmd) {
		case CONFIG_SWITCH_INVALID:
			if (extended_switch_config != EXTENDED_CONFIG_SWITCH_INVALID)
				goto extended_ioctls;
			print_help();
			return -1;
		break;

		case CONFIG_SWITCH_ADD_MULTICAST:
			if ((cmd_struct.vid <= 4095) &&
				(port_num > 0) && (port_num <= 7) &&
				is_multicast_ether_addr(cmd_struct.addr)) {
				cmd_struct.port = port_num;
				if (ioctl(sockfd, SIOCSWITCHCONFIG, &ifr) < 0) {
					printf("Multicast add failed\n");
					close(sockfd);
					return -1;
				} else
					printf("Multicast address added successfully\n");
			} else {
				printf("Invalid Arguments\n");
				return -1;
			}
		break;

		case CONFIG_SWITCH_DEL_MULTICAST:
			if ((cmd_struct.vid <= 4095) &&
				is_multicast_ether_addr(cmd_struct.addr)) {
				if (ioctl(sockfd, SIOCSWITCHCONFIG, &ifr) < 0) {
					printf("Multicast Address Not found\n");
					close(sockfd);
					return -1;
				} else
					printf("Multicast Address Deleted successfully\n");
			} else {
				printf("Invalid Arguments\n");
				return -1;
			}
		break;

		case CONFIG_SWITCH_ADD_VLAN:
			if ((cmd_struct.vid <= 4095) &&
				(port_num > 0) && (port_num <= 7)) {
				cmd_struct.port = port_num;
				if (ioctl(sockfd, SIOCSWITCHCONFIG, &ifr) < 0) {
					printf("VLAN add failed\n");
					close(sockfd);
					return -1;
				} else
					printf("VLAN added successfully\n");
			} else {
				printf("Invalid Arguments\n");
				return -1;
			}
		break;

		case CONFIG_SWITCH_DEL_VLAN:
			if ((cmd_struct.vid <= 4095) &&
				(port_num <= 7)) {
				cmd_struct.port = port_num;
				if (ioctl(sockfd, SIOCSWITCHCONFIG, &ifr) < 0) {
					printf("VLAN Not found\n");
					close(sockfd);
					return -1;
				} else
					printf("VLAN Deleted Successfully\n");
			} else {
				printf("Invalid Arguments\n");
				return -1;
			}
		break;

		case CONFIG_SWITCH_SET_PORT_CONFIG:
			if ((port_num == 1) || (port_num == 2)) {
				cmd_struct.port = port_num;
				cmd_struct.cmd = CONFIG_SWITCH_GET_PORT_CONFIG;

				if (ioctl(sockfd, SIOCSWITCHCONFIG, &ifr) < 0) {
					printf("Get Port Config Failed\n");
					close(sockfd);
					return -1;
				}

				ethtool_cmd_speed_set(&cmd_struct.ecmd, speed);
				cmd_struct.ecmd.duplex = duplex;
				cmd_struct.ecmd.autoneg = autoneg;
				cmd_struct.cmd = CONFIG_SWITCH_SET_PORT_CONFIG;

				if (ioctl(sockfd, SIOCSWITCHCONFIG, &ifr) < 0) {
					printf("Set Port Config Failed\n");
					close(sockfd);
					return -1;
				} else
					printf("Set Port Config successful\n");
			} else {
				printf("Invalid Arguments\n");
				return -1;
			}
		break;

		case CONFIG_SWITCH_GET_PORT_CONFIG:
			if ((port_num == 1) || (port_num == 2)) {
				cmd_struct.port = port_num;
				if (ioctl(sockfd, SIOCSWITCHCONFIG, &ifr) < 0) {
					printf("Get Port Config Failed\n");
					close(sockfd);
					return -1;
				} else {
					printf("Get Port Config successful\n");
					dump_ecmd(&cmd_struct.ecmd);
				}
			} else {
				printf("Invalid Arguments\n");
				return -1;
			}
		break;

		case CONFIG_SWITCH_ADD_UNKNOWN_VLAN_INFO:
			if ((cmd_struct.unknown_vlan_member <= 7) &&
			    (cmd_struct.unknown_vlan_untag <= 7) &&
			    (cmd_struct.unknown_vlan_unreg_multi <= 7) &&
			    (cmd_struct.unknown_vlan_reg_multi <= 7)) {
				if (ioctl(sockfd, SIOCSWITCHCONFIG, &ifr) < 0) {
					printf("Add unknown VLAN Failed\n");
					close(sockfd);
					return -1;
				} else {
					printf("Add unknown VLAN successful\n");
				}
			} else {
				printf("Invalid Arguments\n");
				return -1;
			}
		break;

		case CONFIG_SWITCH_SET_PORT_STATE:
			if ((port_num == 1) || (port_num == 2)) {
				cmd_struct.port = port_num;
				if (ioctl(sockfd, SIOCSWITCHCONFIG, &ifr) < 0) {
					printf("Set Port State Failed\n");
					close(sockfd);
					return -1;
				} else
					printf("Set Port State successful\n");
			} else {
				printf("Invalid Arguments\n");
				return -1;
			}
		break;

		case CONFIG_SWITCH_GET_PORT_STATE:
			if ((port_num == 1) || (port_num == 2)) {
				cmd_struct.port = port_num;
				if (ioctl(sockfd, SIOCSWITCHCONFIG, &ifr) < 0) {
					printf("Get Port State Failed\n");
					close(sockfd);
					return -1;
				} else
					printf("Get Port State successful\nState is ");
				if (cmd_struct.port_state == PORT_STATE_DISABLED)
					printf("disabled\n");
				else if (cmd_struct.port_state == PORT_STATE_BLOCKED)
					printf("blocked\n");
				else if (cmd_struct.port_state == PORT_STATE_LEARN)
					printf("learn\n");
				else if (cmd_struct.port_state == PORT_STATE_FORWARD)
					printf("forward\n");
				else
					printf("Invalid State\n");
			} else {
				printf("Invalid Arguments\n");
				return -1;
			}
		break;

		case CONFIG_SWITCH_GET_PORT_VLAN_CONFIG:
			if (port_num <= 2) {
				cmd_struct.port = port_num;
				if (ioctl(sockfd, SIOCSWITCHCONFIG, &ifr) < 0) {
					printf("Get Port VLAN Failed\n");
					close(sockfd);
					return -1;
				} else {
					printf("Get Port VLAN successful\n");
				}
				printf("Port VLAN id = %d\n", cmd_struct.vid);
				printf("Port VLAN Priority = %d\n", cmd_struct.prio);
				printf("Port VLAN CFI is %sset\n", cmd_struct.vlan_cfi ? "" : "not ");
			} else {
				printf("Invalid Arguments\n");
				return -1;
			}
		break;

		case CONFIG_SWITCH_SET_PORT_VLAN_CONFIG:
			if ((port_num <= 2) && (cmd_struct.prio <= 7)) {
				cmd_struct.port = port_num;
				if (ioctl(sockfd, SIOCSWITCHCONFIG, &ifr) < 0) {
					printf("Set Port VLAN Failed\n");
					close(sockfd);
					return -1;
				} else {
					printf("Set Port VLAN successful\nState is ");
				}
			} else {
				printf("Invalid Arguments\n");
				return -1;
			}
		break;
	}

	close(sockfd);
	return 0;

extended_ioctls:
	switch (extended_switch_config) {
		case EXTENDED_CONFIG_SWITCH_DUMP:
		{
			struct ethtool_drvinfo drvinfo;
			struct ethtool_regs *regs;

			drvinfo.cmd = ETHTOOL_GDRVINFO;
			ifr.ifr_data = (void *)&drvinfo;
			if (ioctl(sockfd, SIOCETHTOOL, &ifr) < 0) {
				perror("Cannot get driver information");
				return -1;
			}

			regs = calloc(1, sizeof(*regs)+drvinfo.regdump_len);
			if (!regs) {
				perror("Cannot allocate memory for register dump");
				return -1;
			}

			regs->cmd = ETHTOOL_GREGS;
			regs->len = drvinfo.regdump_len;
			ifr.ifr_data = (void *)regs;
			if (ioctl(sockfd, SIOCETHTOOL, &ifr) < 0) {
				perror("Cannot get driver information");
				return -1;
			}
			cpsw_dump_regs(&drvinfo, regs);
		}
		break;

		case EXTENDED_CONFIG_SWITCH_INVALID:
		default:
			print_help();
			return -1;
		break;
	}

	close(sockfd);
	return 0;
}
