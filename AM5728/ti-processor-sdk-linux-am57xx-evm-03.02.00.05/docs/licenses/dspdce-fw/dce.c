/*
 * Copyright (c) 2011, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ti/grcm/RcmServer.h>
#include <ti/grcm/RcmTypes.h>
#include <ti/ipc/mm/MmServiceMgr.h>
#include <ti/ipc/mm/MmRpc.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/rpmsg/RPMessage.h>
#include <ti/ipc/rpmsg/NameMap.h>
#include <ti/pm/IpcPower.h>
#include <ti/sdo/ce/global/CESettings.h>
#include <ti/sdo/ce/CERuntime.h>
#include <ti/sdo/ce/Engine.h>
#include <ti/sdo/ce/video2/viddec2.h>
#include <ti/sdo/ce/universal/universal.h>
#include <ti/sdo/fc/global/FCSettings.h>
#include <ti/sdo/fc/utils/fcutils.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/hal/Cache.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/xdais/dm/xdm.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Diags.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/IHeap.h>
#include <xdc/runtime/knl/Thread.h>
#include <xdc/std.h>
#include <ti/sysbios/utils/Load.h>

#include "dce_priv.h"
#include "dce_rpc.h"

static uint32_t    suspend_initialised = 0;
uint32_t           dce_debug = DCE_DEBUG_LEVEL;

#define SERVER_NAME "rpmsg-dce-dsp"
#define MEMORYSTATS_DEBUG

#define MmRpc_NUM_PARAMETERS(size) \
    (size / sizeof(MmType_Param))

/* dce_inv, dce_clean needs to be modified to expect buffers */
/* without headers (relevant for GLP                                    */
static void dce_inv(void *ptr)
{
    if( ptr ) {
        Cache_inv(ptr, P2H(ptr)->size, Cache_Type_ALL, TRUE);
    }
}

static void dce_clean(void *ptr)
{
    if( ptr ) {
        Cache_wbInv(ptr, P2H(ptr)->size, Cache_Type_ALL, TRUE);
    }
}

typedef void * (*CreateFxn)(Engine_Handle, String, void *);
typedef Int32 (*ControlFxn)(void *, int, void *, void *);
typedef Int32 (*ProcessFxn)(void *, void *, void *, void *, void *);
typedef Int32 (*RelocFxn)(void *, uint8_t *ptr, uint32_t len);
typedef void (*DeleteFxn)(void *);

/* DCE Server static function declarations */
static Int32 engine_open(UInt32 size, UInt32 *data);
static Int32 engine_close(UInt32 size, UInt32 *data);
static Int32 get_rproc_info(UInt32 size, UInt32 *data);

/* VIDDEC2 Decoder Server static function declarations */
static VIDDEC2_Handle viddec2_create(Engine_Handle engine, String name, VIDDEC2_Params *params);
static int viddec2_reloc(VIDDEC2_Handle handle, uint8_t *ptr, uint32_t len);
static int viddec2_control(VIDDEC2_Handle handle, VIDDEC2_Cmd id, VIDDEC2_DynamicParams *dynParams,
                              VIDDEC2_Status *status );
static int viddec2_process(VIDDEC2_Handle handle, XDM1_BufDesc *inBufs,
                         XDM_BufDesc *outBufs, VIDDEC2_InArgs *inArgs, VIDDEC2_OutArgs *outArgs );
static int viddec2_delete(VIDDEC2_Handle handle);

static struct {
    CreateFxn  create;
    ControlFxn control;
    ProcessFxn process;
    DeleteFxn  delete;
    RelocFxn   reloc;   /* handle buffer relocation table */
} codec_fxns[] =
{
    [OMAP_DCE_VIDDEC2] =
    {
        (CreateFxn)viddec2_create,   (ControlFxn)viddec2_control,
        (ProcessFxn)viddec2_process, (DeleteFxn)viddec2_delete,
        (RelocFxn)viddec2_reloc,
    },
};

#define UNIVERSAL_COPY_EXAMPLE
/* Static version string buffer.
 * Note: codec version can be large. For example, h264vdec needs more than
 * 58 characters, or the query will fail. */
#define VERSION_SIZE 128
static char    version_buffer[VERSION_SIZE];

/* the following callbacks are needed for suspend/resume
 * on the linux side.
 * - FC_suspend() waits for all algorithms to get deactivated and
 * and takes care of the resources acquired.
 * - FC_resume() does nothing for now, but we add it just in case
 * it gets populated in the future versions of framework components.
 *
 * Forced off mode during video decode/encode is not supported. */
#if 0
static void dce_suspend()
{
    INFO("Preparing for suspend...");
    FC_suspend();
}

static void dce_resume()
{
    INFO("Restoring after suspend...");
    FC_resume();
}
#endif

#ifdef UNIVERSAL_COPY_EXAMPLE
static void get_universal_version(UNIVERSAL_Handle h, char *buffer, unsigned size)
{
    UNIVERSAL_DynamicParams    params =
    {
        .size = sizeof(UNIVERSAL_DynamicParams),
    };

    UNIVERSAL_Status    status =
    {
        .size = sizeof(UNIVERSAL_Status),
        .data =
        {
		    .numBufs = 1,
            .descs[0].buf = (XDAS_Int8 *)buffer,
            .descs[0].bufSize = (XDAS_Int32)size,
        },
    };

    XDAS_Int32    s;

    memset(buffer, 0, size);
    s = UNIVERSAL_control(h, XDM_GETVERSION, &params, &status);

    if( s != IUNIVERSAL_EOK ) {
        ERROR("Unknown version Error = %d:: buffer = %p size = %d", s, buffer, size);
    }
}
#else
static void get_viddec2_version(VIDDEC2_Handle h, char *buffer, unsigned size)
{
    VIDDEC2_DynamicParams    params =
    {
        .size = sizeof(VIDDEC2_DynamicParams),
    };

    VIDDEC2_Status    status =
    {
        .size = sizeof(VIDDEC2_Status),
        .data =
        {
            .buf = (XDAS_Int8 *)buffer,
            .bufSize = (XDAS_Int32)size,
        },
    };

    XDAS_Int32    s;

    memset(buffer, 0, size);
    s = VIDDEC2_control(h, XDM_GETVERSION, &params, &status);

    if( s != VIDDEC2_EOK ) {
        ERROR("Unknown version Error = %d:: buffer = %p size = %d", s, buffer, size);
    }
}

#endif

// VIDDEC2_create wrapper, to display version string in the trace.
static VIDDEC2_Handle viddec2_create(Engine_Handle engine, String name, VIDDEC2_Params *params)
{
#ifdef UNIVERSAL_COPY_EXAMPLE
    UNIVERSAL_Handle   h;

    DEBUG(">> engine=%08x, name=%s, params=%p", engine, name, params);

    h = UNIVERSAL_create(engine, name, (IUNIVERSAL_Params*)params);
    if(h){
    	get_universal_version(h, version_buffer, VERSION_SIZE);
        INFO("Created codec %s: version %s", name, version_buffer);
      }
#else
    VIDDEC2_Handle    h;

    h = VIDDEC2_create(engine, name, params);

    if( h ) {
        get_viddec2_version(h, version_buffer, VERSION_SIZE);
        INFO("Created viddec2 %s: version %s", name, version_buffer);
    }
#endif
    return ((VIDDEC2_Handle)h);
}

/* needs to be updated when XDM_MOVEBUFS added in XDC tools */
static int viddec2_reloc(VIDDEC2_Handle handle, uint8_t *ptr, uint32_t len)
{
    return (-1); // TODO

}

static int viddec2_control(VIDDEC2_Handle handle, VIDDEC2_Cmd id, VIDDEC2_DynamicParams *dynParams,
                              VIDDEC2_Status *status )
{
   int ret = 0;
#ifdef UNIVERSAL_COPY_EXAMPLE
   UNIVERSAL_DynamicParams udynParam;
   UNIVERSAL_Status ustatus;

   udynParam.size = sizeof(UNIVERSAL_DynamicParams);
   ustatus.size = sizeof(UNIVERSAL_Status);

   if(id == XDM_GETVERSION){
       ustatus.data.numBufs = 1;
	   ustatus.data.descs[0].buf = status->data.buf;
	   ustatus.data.descs[0].bufSize = status->data.bufSize;
     }

   ret = UNIVERSAL_control((UNIVERSAL_Handle)handle, (UNIVERSAL_Cmd)id,
                              &udynParam, &ustatus);

   /*universal copy supports only XDM_GETVERSION cmd id    */
   /*This is to return success to VIDDEC2 application in case of other cmd ids */
   if(ret == IUNIVERSAL_EFAIL)ret = IUNIVERSAL_EOK;
#else
   ret = VIDDEC2_control(handle, id, dynParams, status);
#endif
   return ret;
}

static int viddec2_process(VIDDEC2_Handle handle, XDM1_BufDesc *inBufs,
                         XDM_BufDesc *outBufs, VIDDEC2_InArgs *inArgs, VIDDEC2_OutArgs *outArgs )
{
     int ret = 0;
#ifdef UNIVERSAL_COPY_EXAMPLE
	 XDM1_BufDesc inBuf, outBuf;
	 UNIVERSAL_InArgs inArg;
	 UNIVERSAL_OutArgs outArg;

     inArg.size = sizeof(UNIVERSAL_InArgs);
     outArg.size = sizeof(UNIVERSAL_OutArgs);
     outArg.extendedError = 0;

	 inBuf.numBufs = 1;
	 outBuf.numBufs = 1;
	 inBuf.descs[0].buf = inBufs->descs[0].buf;
	 inBuf.descs[0].bufSize = inBufs->descs[0].bufSize;

	 outBuf.descs[0].buf = outBufs->bufs[0];
	 outBuf.descs[0].bufSize = outBufs->bufSizes[0];

     ret = UNIVERSAL_process((UNIVERSAL_Handle)handle, &inBuf, &outBuf, NULL,
	                        &inArg, &outArg);
#else
    ret = VIDDEC2_process(handle, inBufs, outBufs, inArgs, outArgs);
#endif
	  return ret;
}

static int viddec2_delete(VIDDEC2_Handle handle)
{
#ifdef UNIVERSAL_COPY_EXAMPLE
   UNIVERSAL_delete((UNIVERSAL_Handle)handle);
#else
   VIDDEC2_delete(handle);
#endif
   return 0;
}


/*
 * RPC message handlers
 */
static int connect(void *msg)
{
    //dce_connect   *req = msg;
    //DEBUG(">> chipset_id=0x%x, debug=%d", req->chipset_id, req->debug);

    if( dce_debug >= MAX_DEBUG_LEVEL ) {
        DEBUG("Enable FC, CE and IPC traces");

        FCSettings_init();
        Diags_setMask(FCSETTINGS_MODNAME "+12345678LEXAIZFS");

		CESettings_init();
        Diags_setMask(CESETTINGS_MODNAME "+12345678LEXAIZFS");

        /*
            * Enable use of runtime Diags_setMask per module:
            *
            * Codes: E = ENTRY, X = EXIT, L = LIFECYCLE, F = INFO, S = STATUS
            */
        Diags_setMask("ti.ipc.rpmsg.RPMessage=EXLFS");
        Diags_setMask("ti.ipc.rpmsg.VirtQueue=EXLFS");
    }

	  CERuntime_init();

      if( !suspend_initialised ) {

        /* registering sysbios-rpmsg callbacks for suspend/resume */
     //   IpcPower_registerCallback(IpcPower_Event_SUSPEND, (IpcPower_CallbackFuncPtr)dce_suspend, 0);
     //   IpcPower_registerCallback(IpcPower_Event_RESUME, (IpcPower_CallbackFuncPtr)dce_resume, 0);
        suspend_initialised++;
    }

    DEBUG("<<");

    return (0);
}

/*
 * Engine_open:
 */
static Int32 engine_open(UInt32 size, UInt32 *data)
{
    MmType_Param      *payload = (MmType_Param *)data;
    dce_engine_open   *engine_open_msg = (dce_engine_open *)payload[0].data;
    Engine_Handle      eng_handle = NULL;
    Uint32             num_params = MmRpc_NUM_PARAMETERS(size);

    DEBUG(">> engine_open");
    if( num_params != 1 ) {
        ERROR("Invalid number of params sent");
        return (-1);
    }

    dce_inv(engine_open_msg);

	eng_handle = Engine_open(engine_open_msg->name, engine_open_msg->engine_attrs, &engine_open_msg->error_code);
	DEBUG("<< engine=%08x, ec=%d", eng_handle, engine_open_msg->error_code);

    dce_clean(engine_open_msg);

    return ((Int32)eng_handle);
}

/*
 * Engine_close:
 */
static Int32 engine_close(UInt32 size, UInt32 *data)
{
    MmType_Param    *payload = (MmType_Param *)data;
    Engine_Handle    eng_handle = (Engine_Handle)payload[0].data;
    Uint32           num_params = MmRpc_NUM_PARAMETERS(size);

    if( num_params != 1 ) {
        ERROR("invalid number of params sent");
        return (-1);
    }

    Engine_close(eng_handle);
    DEBUG("<<");

    return (0);
}

#define INFO_TYPE_CPU_LOAD 0
#define INFO_TYPE_TOTAL_HEAP_SIZE 1
#define INFO_TYPE_AVAILABLE_HEAP_SIZE 2

static Int32 get_rproc_info(UInt32 size, UInt32 *data)
{
    MmType_Param    *payload = (MmType_Param *)data;
    Uint32           info_type = (Uint32)payload[0].data;
    Uint32 output = 0;
    Memory_Stats    stats;

    switch(info_type)
    {
            case INFO_TYPE_CPU_LOAD:
               output = Load_getCPULoad();
                break;

            case INFO_TYPE_TOTAL_HEAP_SIZE:
                Memory_getStats(NULL, &stats);
                output = stats.totalSize;
                break;

            case INFO_TYPE_AVAILABLE_HEAP_SIZE:
                Memory_getStats(NULL, &stats);
                output = stats.totalFreeSize;
                break;

            default:
                System_printf("\n ERROR: Invalid INFO TYPE chosen \n");
                break;
    }

    return output;
}


/*
  * codec_create
  */
static Int32 codec_create(UInt32 size, UInt32 *data)
{
    MmType_Param    *payload = (MmType_Param *)data;
    Uint32           codec_id      = (Uint32)payload[0].data;
    Engine_Handle    engine = (Engine_Handle)payload[1].data;
    char            *codec_name    = (char *)payload[2].data;
    void            *static_params = (void *)payload[3].data;
    Uint32           num_params = MmRpc_NUM_PARAMETERS(size);
    void            *codec_handle;

#ifdef MEMORYSTATS_DEBUG
    Memory_Stats    stats;
#endif
    DEBUG(">> codec_create");

    if( num_params != 4 ) {
        ERROR("invalid number of params sent");
        return (-1);
    }

	if(codec_id != OMAP_DCE_VIDDEC2){
        ERROR("invalid codec id sent");
        return (-1);
    }

    dce_inv(static_params);

    codec_handle = (void *)codec_fxns[codec_id].create(engine, codec_name, static_params);

    DEBUG("<< codec_handle=%08x", codec_handle);

    dce_clean(static_params);

#ifdef MEMORYSTATS_DEBUG
    Memory_getStats(NULL, &stats);
    INFO("Total: %d\tFree: %d\tLargest: %d", stats.totalSize,
         stats.totalFreeSize, stats.largestFreeSize);
#endif
    return ((Int32)codec_handle);
}

/*
  * codec_control
  */
static int codec_control(UInt32 size, UInt32 *data)
{
    MmType_Param   *payload = (MmType_Param *)data;
    Uint32          codec_id            = (Uint32)payload[0].data;
    void           *codec_handle = (Engine_Handle)payload[1].data;
    uint32_t        cmd_id              = (Uint32)payload[2].data;
    void           *dyn_params          = (void *)payload[3].data;
    void           *status              = (void *)payload[4].data;
    Uint32          num_params = MmRpc_NUM_PARAMETERS(size);
    Int32           ret = 0;


    DEBUG(">> codec_control");

    if( num_params != 5 ) {
        ERROR("invalid number of params sent");
        return (-1);
    }

	if(codec_id != OMAP_DCE_VIDDEC2){
        ERROR("invalid codec id sent");
        return (-1);
    }

    dce_inv(dyn_params);
    dce_inv(status);

    ret = (uint32_t) codec_fxns[codec_id].control(codec_handle, cmd_id, dyn_params, status);

    DEBUG("<< result=%d", ret);

    dce_clean(dyn_params);
    dce_clean(status);

    return (ret);
}

/*
  * codec get version
  */
static int codec_get_version(UInt32 size, UInt32 *data)
{
    MmType_Param   *payload = (MmType_Param *)data;
    Uint32          codec_id            = (Uint32)payload[0].data;
    void           *codec_handle = (Engine_Handle)payload[1].data;
    void           *dyn_params          = (void *)payload[2].data;
    void           *status              = (void *)payload[3].data;
    Uint32          num_params = MmRpc_NUM_PARAMETERS(size);
    void           *version_buf = NULL;
    Int32           ret = 0;

    DEBUG(">> codec_get_version");

    if( num_params != 4 ) {
        ERROR("invalid number of params sent");
        return (-1);
    }

	if(codec_id != OMAP_DCE_VIDDEC2){
        ERROR("invalid codec id sent");
        return (-1);
    }

    version_buf = (void *)(H2P((MemHeader *)((IVIDDEC2_Status *)status)->data.buf));

    dce_inv(dyn_params);
    dce_inv(status);
    dce_inv(version_buf);

    ret = (uint32_t) codec_fxns[codec_id].control(codec_handle, XDM_GETVERSION, dyn_params, status);

    DEBUG("<< result=%d", ret);

    dce_clean(dyn_params);
    dce_clean(status);
    dce_clean(version_buf);

    return (ret);
}

/* Notes about serialization of process command:
 *
 * Since codec_process code on kernel side is doing buffer mapping/unmapping,
 * and keeping track of codec's locked buffers, it is necessary for it to
 * look into the contents of some of the parameter structs, and in some cases
 * re-write them.  For this reason inArgs/outBufs/inBufs are serialized within
 * the rpmsg rather than just passed by pointer.

XDAS_Int32 VIDDEC3_process(VIDDEC3_Handle handle, XDM1_BufDesc *inBufs,
    XDM_BufDesc *outBufs, VIDDEC3_InArgs *inArgs, VIDDEC3_OutArgs *outArgs);

  REQ:
    struct dce_rpc_hdr hdr   -> 4
    codec_id                 -> 4
    codec                    -> 4
    reloc length             -> 1   (length/4)
    inArgs length            -> 1   (length/4)
    outBufs length           -> 1   (length/4)
    inBufs length            -> 1   (length/4)
    VIDDEC3_OutArgs *outArgs -> 4   (pass by pointer)
    reloc table              -> 12 * nreloc (typically <= 16)
    VIDDEC3_InArgs   inArgs  -> 12  (need inputID from userspace)
    XDM_BufDesc     outBufs -> 44  (4 + 2 * 20)
    XDM1_BufDesc     inBufs  -> 24  (4 + 1 * 20)
    -------------------------------
                               99

  RSP
    struct dce_rpc_hdr hdr   -> 4
    result                   -> 4
    inBufs length            -> 1   (length/4)
    XDAS_Int32 freeBufID[]   -> 4*n (n typically 0 or 2, but could be up to 20)
    -------------------------------
                               9-89
    Note: freeBufID[] duplicates what is returned in outArgs, but avoids
    needing to create kernel mappings of these objects which are to big
    to copy inline.  Also it avoids differences between VIDDEC3/VIDDENC2.


XDAS_Int32 VIDENC2_process(VIDENC2_Handle handle, IVIDEO2_BufDesc *inBufs,
    XDM2_BufDesc *outBufs, IVIDENC2_InArgs *inArgs, IVIDENC2_OutArgs *outArgs);

  REQ:
    struct dce_rpc_hdr hdr   -> 4
    codec_id                 -> 4
    codec                    -> 4
    reloc length             -> 1   (length/4)
    inArgs length            -> 1   (length/4)
    outBufs length           -> 1   (length/4)
    inBufs length            -> 1   (length/4)
    VIDENC2_OutArgs *outArgs -> 4   (pass by pointer)
    reloc table              -> ???
    VIDENC2_InArgs   inArgs  -> 12  (need inputID from userspace)
    XDM2_BufDesc     outBufs -> 24  (4 + 1 * 20)
    IVIDEO2_BufDesc  inBufs  -> 252
    -------------------------------
                              307

  RSP
    struct dce_rpc_hdr hdr   -> 4
    result                   -> 4
    inBufs length            -> 1   (length/4)
    XDAS_Int32 freeBufID[]   -> 4*n (n typically 0 or 2, but could be up to 20)
    -------------------------------
                               9-89
 */

static int codec_process(UInt32 size, UInt32 *data)
{
    MmType_Param   *payload = (MmType_Param *)data;
    Uint32          num_params = MmRpc_NUM_PARAMETERS(size);
    Uint32          codec_id = (Uint32) payload[0].data;
    Uint32          codec    = (Uint32) payload[1].data;
    void           *inBufs   = (void *) payload[2].data;
    void           *outBufs  = (void *) payload[3].data;
    void           *inArgs   = (void *) payload[4].data;
    void           *outArgs  = (void *) payload[5].data;
	void           *outBufptr  = (void *)payload[6].data;
    Int32           ret = 0;
    void           *outBufSize = NULL;

    DEBUG(">> codec_process");

    if( num_params != 7 ) {
        ERROR("invalid number of params sent");
        return (-1);
    }

	if(codec_id != OMAP_DCE_VIDDEC2){
        ERROR("invalid codec id sent");
        return (-1);
    }

	outBufSize = (void *)(H2P((MemHeader *)((XDM_BufDesc *)outBufs)->bufSizes));

    dce_inv(inBufs);
    dce_inv(outBufs);
    dce_inv(inArgs);
    dce_inv(outArgs);
	dce_inv(outBufptr);
    dce_inv(outBufSize);

    DEBUG(">> codec=%p, inBufs=%p, outBufs=%p, inArgs=%p, outArgs=%p codec_id=%d",
          codec, inBufs, outBufs, inArgs, outArgs, codec_id);

    ret = codec_fxns[codec_id].process((void *)codec, inBufs, outBufs, inArgs, outArgs);


    DEBUG("<< ret=%d extendedError=%08x", ret, ((VIDDEC3_OutArgs *)outArgs)->extendedError);

    dce_clean(inBufs);
    dce_clean(outBufs);
    dce_clean(inArgs);
    dce_clean(outArgs);
	dce_clean(outBufptr);
	dce_clean(outBufSize);

    return ((Int32)ret);
}

/*
  * codec delete
  */

static int codec_delete(UInt32 size, UInt32 *data)
{
    MmType_Param   *payload = (MmType_Param *)data;
    Uint32          num_params = MmRpc_NUM_PARAMETERS(size);
    Uint32          codec_id = (Uint32) payload[0].data;
    Uint32          codec    = (Uint32) payload[1].data;

#ifdef MEMORYSTATS_DEBUG
    Memory_Stats    stats;
#endif

    DEBUG(">> codec_delete");

    if( num_params != 2 ) {
        ERROR("invalid number of params sent");
        return (-1);
    }

	if(codec_id != OMAP_DCE_VIDDEC2){
        ERROR("invalid codec id sent");
        return (-1);
    }

    codec_fxns[codec_id].delete((void *)codec);

#ifdef MEMORYSTATS_DEBUG
    Memory_getStats(NULL, &stats);
    INFO("Total: %d\tFree: %d\tLargest: %d", stats.totalSize,
         stats.totalFreeSize, stats.largestFreeSize);
#endif

    DEBUG("<<");

    return (0);
}

/* the server create parameters, must be in persistent memory */
static RcmServer_Params    rpc_Params;

/* DCE Server skel function array */
static RcmServer_FxnDesc    DCEServerFxnAry[] =
{
    { "engine_open",       (RcmServer_MsgFxn) engine_open },
    { "engine_close",      (RcmServer_MsgFxn) engine_close },
    { "codec_create",      (RcmServer_MsgFxn) codec_create },
    { "codec_control",     (RcmServer_MsgFxn) codec_control },
    { "codec_get_version", (RcmServer_MsgFxn) codec_get_version },
    { "codec_process",     (RcmServer_MsgFxn) codec_process },
    { "codec_delete",      (RcmServer_MsgFxn) codec_delete },
    { "get_rproc_info",    (RcmServer_MsgFxn) get_rproc_info }
};

/* DCE Server skel function table */
#define DCEServerFxnAryLen (sizeof(DCEServerFxnAry) / sizeof(DCEServerFxnAry[0]))

static const RcmServer_FxnDescAry    DCEServer_fxnTab =
{
    DCEServerFxnAryLen,
    DCEServerFxnAry
};

static MmType_FxnSig    DCEServer_sigAry[] =
{
    { "engine_open", 2,
      {
          { MmType_Dir_Out, MmType_Param_S32, 1 }, // return
          { MmType_Dir_Bi, MmType_PtrType(MmType_Param_VOID), 1 }
      } },
    { "engine_close", 2,
      {
          { MmType_Dir_Out, MmType_Param_S32, 1 }, // return
          { MmType_Dir_In, MmType_Param_U32, 1 }

      } },
    { "codec_create", 5,
      {
          { MmType_Dir_Out, MmType_Param_S32, 1 }, // return
          { MmType_Dir_In, MmType_Param_U32, 1 },
          { MmType_Dir_In, MmType_Param_U32, 1 },
          { MmType_Dir_In, MmType_PtrType(MmType_Param_VOID), 1 },
          { MmType_Dir_In, MmType_PtrType(MmType_Param_VOID), 1 }
      } },
    { "codec_control", 6,
      {
          { MmType_Dir_Out, MmType_Param_S32, 1 }, // return
          { MmType_Dir_In, MmType_Param_U32, 1 },
          { MmType_Dir_In, MmType_Param_U32, 1 },
          { MmType_Dir_In, MmType_Param_U32, 1 },
          { MmType_Dir_In, MmType_PtrType(MmType_Param_VOID), 1 },
          { MmType_Dir_Bi, MmType_PtrType(MmType_Param_VOID), 1 }
      } },
    { "codec_get_version", 5,
      {
          { MmType_Dir_Out, MmType_Param_S32, 1 }, // return
          { MmType_Dir_In, MmType_Param_U32, 1 },
          { MmType_Dir_In, MmType_Param_U32, 1 },
          { MmType_Dir_In, MmType_PtrType(MmType_Param_VOID), 1 },
          { MmType_Dir_Bi, MmType_PtrType(MmType_Param_VOID), 1 }
      } },

     { "codec_process", 8,
      {
          { MmType_Dir_Out, MmType_Param_S32, 1 }, // return
          { MmType_Dir_In, MmType_Param_U32, 1 },
          { MmType_Dir_In, MmType_Param_U32, 1 },
          { MmType_Dir_Bi, MmType_PtrType(MmType_Param_VOID), 1 },
          { MmType_Dir_Bi, MmType_PtrType(MmType_Param_VOID), 1 },
          { MmType_Dir_Bi, MmType_PtrType(MmType_Param_VOID), 1 },
          { MmType_Dir_Bi, MmType_PtrType(MmType_Param_VOID), 1 },
		  { MmType_Dir_Bi, MmType_PtrType(MmType_Param_VOID), 1 }
      } },

    { "codec_delete", 3,
      {
          { MmType_Dir_Out, MmType_Param_S32, 1 }, // return
          { MmType_Dir_In, MmType_Param_U32, 1 },
          { MmType_Dir_In, MmType_Param_U32, 1 }
     } },
    { "get_rproc_info", 2,
      {
         { MmType_Dir_Out, MmType_Param_S32, 1 }, // return
          { MmType_Dir_In, MmType_Param_U32, 1 }
     } }
};

static MmType_FxnSigTab    dce_fxnSigTab =
{
    MmType_NumElem(DCEServer_sigAry), DCEServer_sigAry
};

Void dce_SrvDelNotification(Void)
{
    DEBUG("dce_SrvDelNotification: Nothing to cleanup\n");
}

static void dce_main(uint32_t arg0, uint32_t arg1)
{
    int            err = 0;
    dce_connect    dce_connect_msg;

    err = MmServiceMgr_init();  // MmServiceMgr_init() will always return MmServiceMgr_S_SUCCESS.

    // setup the RCM Server create params
    RcmServer_Params_init(&rpc_Params);
    rpc_Params.priority = Thread_Priority_ABOVE_NORMAL;
    rpc_Params.stackSize = 0x1000;
    rpc_Params.fxns.length = DCEServer_fxnTab.length;
    rpc_Params.fxns.elem = DCEServer_fxnTab.elem;

    // Get the Service Manager handle
    err = MmServiceMgr_register(SERVER_NAME, &rpc_Params, &dce_fxnSigTab, dce_SrvDelNotification);

    if( err < 0 ) {
        DEBUG("failed to start " SERVER_NAME " \n");
		//err = -1;
    } else {
        DEBUG(SERVER_NAME " running through MmServiceMgr");
	}

    // Question to IPC team: where is the call to OmapRpc_deleteChannel(handle). The OmapRpc_createChannel is part of MmServiceMgr_register
    // missing the call to OmapRpc_deleteChannel(handle).
    MmServiceMgr_exit();

    DEBUG("deleted " SERVER_NAME);

    /* Read the register for ID_CODE to figure out the correct configuration: */
    /* CONTROL_STD_FUSE_ID_CODE[31:0] ID_CODE STD_FUSE_IDCODE */
    /* physical address: 0x4A00 2204 Address offset: 0x204                       */
#ifdef OMAP5430_ES10
    dce_connect_msg.chipset_id = 0x5430;
#elif OMAP5432_ES20
    dce_connect_msg.chipset_id = 0x5432;
#elif VAYU_ES10
    dce_connect_msg.chipset_id = 0x5436;
#endif
    dce_connect_msg.debug = dce_debug;
    connect(&dce_connect_msg);

    return;
}

/*
  * dce init : Startup Function
  */
Bool dce_init(void)
{
    Task_Params    params;

    INFO("Creating DCE server thread...");


    /* Create DCE task. */
    Task_Params_init(&params);
    params.instance->name = "dce-server";
    params.priority = Thread_Priority_ABOVE_NORMAL;
	Task_create(dce_main, &params, NULL);

    return (TRUE);
}

/*
  * dce deinit
  */

void dce_deinit(void)
{
    DEBUG("dce_deinit");
}

