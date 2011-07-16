/*
 **    This file is part of LightShoppe. Copyright 2011 Jack Andersen
 **
 **    LightShoppe is free software: you can redistribute it
 **    and/or modify it under the terms of the GNU General
 **    Public License as published by the Free Software
 **    Foundation, either version 3 of the License, or (at your
 **    option) any later version.
 **
 **    LightShoppe is distributed in the hope that it will
 **    be useful, but WITHOUT ANY WARRANTY; without even the
 **    implied warranty of MERCHANTABILITY or FITNESS FOR A
 **    PARTICULAR PURPOSE.  See the GNU General Public License
 **    for more details.
 **
 **    You should have received a copy of the GNU General
 **    Public License along with LightShoppe.  If not, see
 **    <http://www.gnu.org/licenses/>.
 **
 **    @author Jack Andersen <jackoalan@gmail.com>
 */


/*
  * DMX.c
  *
  *  Created on: 19 Dec 2010 Author: Jacko
  */

#include <stdio.h>
#include <math.h>

#include "OLAWrapper.h"

#include "DMX.h"
#include "CorePlugin.h"
#include "DBArr.h"
#include "SceneCore.h"
#include "Node.h"
#include "Logging.h"

/* Gettext stuff */
#include <libintl.h>
#define _(String) gettext (String)

/* Name of this component for logging */
static const char LOG_COMP[] = "DMX.c";

int
initDMX ()
{
    return initOlaClient ();
}


void
closeDMX ()
{
    stopOlaClient ();
}


int
bufferUnivs ()
{

    int rgbType = core_getRGBTypeID ();

    struct LSD_ArrayHead* chanArr = getArr_lsdChannelArr ();

    struct LSD_Channel* chan = NULL;
    struct RGB_TYPE* rgb = NULL;
    unsigned int rVal, gVal, bVal;
    int i;
    if (chanArr->maxIdx == -1)
        return 0;
    for (i = 0; i <= chanArr->maxIdx; ++i)
    {
        if (pickIdx (chanArr, (void**)&chan, i) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to pick channel in bufferUnivs()."));
            return -1;
        }
        if (chan && chan->output && chan->output->typeId == rgbType)
        {

            if (!chan->output->bufferFunc)
                doLog (ERROR, LOG_COMP, _("No buffer func to call on output connected to channel %d."), chan->dbId);

            /* printf("Ran\n"); */

            rgb = node_bufferOutput (chan->output);

            /* Red/Mono */
            rVal = lround (rgb->r * 0xffff);
            chan->rAddr.univ->buffer[chan->rAddr.addr] = rVal >> 8;
            if (chan->rAddr.b16)
                chan->rAddr.univ->buffer[chan->rAddr.addr +
                                         1] = ( rVal ) & 0xff;

            if (!chan->single)
            {
                /* Green */
                gVal = lround (rgb->g * 0xffff);
                chan->gAddr.univ->buffer[chan->gAddr.addr] = gVal >> 8;
                if (chan->gAddr.b16)
                    chan->gAddr.univ->buffer[chan->gAddr.addr +
                                             1] = ( gVal ) & 0xff;

                /* Blue */
                bVal = lround (rgb->b * 0xffff);
                chan->bAddr.univ->buffer[chan->bAddr.addr] = bVal >> 8;
                if (chan->bAddr.b16)
                    chan->bAddr.univ->buffer[chan->bAddr.addr +
                                             1] = ( bVal ) & 0xff;

            }
        }
        else
        {
            /* fprintf(stderr,"Channel's Output isn't
             * connected or isn't standard RGB\n"); */

            /* Red/Mono */
            chan->rAddr.univ->buffer[chan->rAddr.addr] = 0;
            if (chan->rAddr.b16)
                chan->rAddr.univ->buffer[chan->rAddr.addr + 1] = 0;

            if (!chan->single)
            {
                /* Green */
                chan->gAddr.univ->buffer[chan->gAddr.addr] = 0;
                if (chan->gAddr.b16)
                    chan->gAddr.univ->buffer[chan->gAddr.addr + 1] = 0;

                /* Blue */
                chan->bAddr.univ->buffer[chan->bAddr.addr] = 0;
                if (chan->bAddr.b16)
                    chan->bAddr.univ->buffer[chan->bAddr.addr + 1] = 0;

            }
        }
    }

    return 0;

}


int
writeUnivs ()
{

    struct LSD_ArrayHead* univsArr = getArr_lsdUnivArr ();

    struct LSD_Univ* univ = NULL;
    int i;
    if (univsArr->maxIdx == -1)
        return 0;
    for (i = 0; i <= univsArr->maxIdx; ++i)
    {
        if (pickIdx (univsArr, (void**)&univ, i) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to pick Univ in writeUnivs()."));
            return -1;
        }

        updateDMX (univ->buffer, univ->maxIdx, univ->olaUnivId);
        /* printf("MaxIdx: %d\n",univ->maxIdx); */
    }

    return 0;

}


