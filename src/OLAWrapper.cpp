/*
**    This file is part of LightShoppe.
**    Copyright 2011 Jack Andersen
**
**    LightShoppe is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    LightShoppe is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with LightShoppe.  If not, see <http://www.gnu.org/licenses/>.
**
**    @author Jack Andersen <jackoalan@gmail.com>
*/


/*
 * OLAWrapper.cpp
 *
 *  Created on: 2 Jan 2011
 *      Author: Jacko
 */

#include "OLAWrapper.h"

#include <ola/DmxBuffer.h>
#include <ola/StreamingClient.h>

static ola::DmxBuffer dmxbuf;
static ola::StreamingClient ola_client;

int 
initOlaClient (){
    if(!ola_client.Setup ()){
        return -1;
    }
    return 0;
}

int 
olaUpdateDMX (uint8_t* univ, size_t len, int univId){
    dmxbuf.Blackout ();
    dmxbuf.Set (univ+1,len+1);


    ola_client.SendDmx (univId,dmxbuf);
    return 0;
}

void 
stopOlaClient(){
    ola_client.Stop ();
    
    dmxbuf.~DmxBuffer ();
    ola_client.~StreamingClient ();
}
