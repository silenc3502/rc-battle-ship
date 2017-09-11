/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2014 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************
*
*    The GPL License (GPL)
*
*    Copyright (C) 2014  Vivante Corporation
*
*    This program is free software; you can redistribute it and/or
*    modify it under the terms of the GNU General Public License
*    as published by the Free Software Foundation; either version 2
*    of the License, or (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*****************************************************************************
*
*    Note: This software is released under dual MIT and GPL licenses. A
*    recipient may use this file under the terms of either the MIT license or
*    GPL License. If you wish to use only one license not the other, you can
*    indicate your decision by deleting one of the above license notices in your
*    version of this file.
*
*****************************************************************************/


#include "gc_hal_kernel_linux.h"
#include "gc_hal_kernel_platform.h"

#include <linux/pm_runtime.h>
#include <linux/delay.h>
#include <linux/opp.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_data/omap_gcx.h>

static int gcxxx_get_context_loss_count(struct device *dev)
{
    /*
        omap_pm_get_context_loss_count not supported:
        returning with default 1
    */
    return 1;
}
static int gcxxx_scale_dev(struct device *dev, unsigned long val)
{
    /*omap_device_scale(dev, val) is not supported, returning with no-op
    * for now. */
    return 0;
}

static int gcxxx_set_l3_bw(struct device *dev, unsigned long val)
{
    return 0; /*omap_pm_set_min_bus_tput(dev, OCP_INITIATOR_AGENT, val);*/
}
static struct omap_gcx_platform_data omap_gcxxx = {
    .get_context_loss_count = gcxxx_get_context_loss_count,
    .scale_dev = gcxxx_scale_dev,
    .set_bw = gcxxx_set_l3_bw,
};



static const struct of_device_id bb2d_of_match[] = {
    {
        .compatible = "ti,bb2d",
        .data = &omap_gcxxx,
    },
    {},
};


static gctBOOL
_NeedAddDevice(
    IN gckPLATFORM Platform
    )
{
    return gcvTRUE;
}

static gceSTATUS
_AdjustDriver(
    IN gckPLATFORM Platform
    )
{
    struct platform_driver * driver = Platform->driver;
    driver->driver.of_match_table = bb2d_of_match;
    return gcvSTATUS_TRUE;
}

static gceSTATUS
_GetPower(
    IN gckPLATFORM Platform
    )
{
    struct platform_device *pdev;
    pdev = Platform->device;

    pm_runtime_enable(&pdev->dev);
    pm_runtime_get_sync(&pdev->dev);

    return gcvSTATUS_TRUE;
}

static gceSTATUS
_AdjustParam(
    IN gckPLATFORM Platform,
    OUT gcsMODULE_PARAMETERS *Args
    )
{
    struct resource *res_ptr = NULL;

    struct omap_gcx_platform_data *plat;
    struct of_device_id *match;
    struct platform_device *pdev;

    if (Args->registerMemMapped)
        return gcvSTATUS_TRUE;

    pdev = Platform->device;


    plat = (struct omap_gcx_platform_data *)dev_get_platdata(&pdev->dev);

    match = (struct of_device_id *)of_match_device(of_match_ptr(bb2d_of_match), &pdev->dev);

    if (match) {
        plat = (struct omap_gcx_platform_data *) match->data;
        plat->is_hw_present = (of_machine_is_compatible("ti, omap54xx")	|| of_machine_is_compatible("ti,dra7"));
    }

    if ( !plat->is_hw_present )
        return gcvSTATUS_FALSE;

    res_ptr = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res_ptr) {
        return gcvSTATUS_FALSE;
    }
     Args->registerMemAddress = (gctPOINTER)devm_ioremap_resource(&pdev->dev, res_ptr);
    Args->registerMemMapped = gcvTRUE;
    return gcvSTATUS_TRUE;
}

gcsPLATFORM_OPERATIONS platformOperations =
{
    .adjustParam   = _AdjustParam,
    .adjustDriver = _AdjustDriver,
    .needAddDevice = _NeedAddDevice,
    .getPower      = _GetPower,
};

void
gckPLATFORM_QueryOperations(
    IN gcsPLATFORM_OPERATIONS ** Operations
    )
{
     *Operations = &platformOperations;
}
