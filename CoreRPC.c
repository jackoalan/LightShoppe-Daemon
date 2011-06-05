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


#include "CoreRPC.h"
#include "cJSON.h"

#include <event.h>
#include <evhttp.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "DBOps.h"
#include "SceneCore.h"
#include "PluginAPI.h"


// Core RPC operations

// New Node Instances
// Delete Node Instance
// JSON Node Instances
// {nodes:[{nid:123,name:'meh',desc:'bleh',ins:[{inId:33,name:'inOne',wireId:42/NULL}],outs:[]}]}

// Wire Nodes
// Unwire Nodes
// JSON Wires
// {wires:[{wid:42,out:123,in:456}]}

// JSON Inst Ins

// JSON Inst Outs

// Enable plugin
// Disable plugin
// JSON plugins



// JSON stuff below


void lsdCustomRPC(cJSON* req, cJSON* resp){
	cJSON* nodeId = cJSON_GetObjectItem(req,"nodeId");
	if(nodeId && nodeId->type==cJSON_Number){
		struct LSD_ScenePlugin* plugin;
		if(lsddb_resolvePluginFromNodeId(&plugin,nodeId->valueint)<0){
			cJSON_AddStringToObject(resp,"error","error");
			return;
		}
		
		if(plugin->handleRPC){
			plugin->handleRPC(req,resp);
		}
	}
	else{
		cJSON_AddStringToObject(resp,"error","nodeId not present or not a number");
		return;
	}
}

void lsdJsonLibrary(cJSON* req, cJSON* resp){
    lsddb_jsonClassLibrary(resp);
}

void lsdJsonPartitions(cJSON* req, cJSON* resp){
    lsddb_jsonParts(resp);
}

void lsdJsonPatchSpace(cJSON* req, cJSON* resp){
	cJSON* psId = cJSON_GetObjectItem(req,"psId");
    if(psId && psId->type==cJSON_Number){
		lsddb_jsonPatchSpace(psId->valueint,resp);
    }
    else{
        cJSON_AddStringToObject(resp,"error","psId key not present or not a number");
    }
}

void lsdAddNode(cJSON* req, cJSON* resp){
    cJSON* psId = cJSON_GetObjectItem(req,"psId");
    if(psId && psId->type==cJSON_Number){
        cJSON* classId = cJSON_GetObjectItem(req,"classId");
        if(classId && classId->type==cJSON_Number){
            struct LSD_SceneNodeClass* nc = NULL;
            if(lsddb_resolveClassFromId(&nc,classId->valueint)<0){
                cJSON_AddStringToObject(resp,"error","Problem while resolving class object");
                return;
            }
            int instId;
            if(lsddb_addNodeInst(psId->valueint,nc,&instId,NULL)<0){
                cJSON_AddStringToObject(resp,"error","Unable to add node instance to DB");
            }
            else{
                cJSON_AddStringToObject(resp,"success","success");
                cJSON_AddNumberToObject(resp,"instId",instId);
            }
        }
        else{
            cJSON_AddStringToObject(resp,"error","classId not present or not a number");
        }
    }
    else{
        cJSON_AddStringToObject(resp,"error","psId key not present or not a number");
    }
}

void lsdDeleteNode(cJSON* req, cJSON* resp){
    cJSON* nodeId = cJSON_GetObjectItem(req,"nodeId");
    if(nodeId && nodeId->type==cJSON_Number){
		if(lsddb_removeNodeInst(nodeId->valueint)<0)
			cJSON_AddStringToObject(resp,"error","error");
		else
			cJSON_AddStringToObject(resp,"success","success");
    }
    else{
        cJSON_AddStringToObject(resp,"error","psId key not present or not a number");
    }
}

void lsdAddFacade(cJSON* req, cJSON* resp){
    cJSON* psId = cJSON_GetObjectItem(req,"psId");
	if(psId && psId->type==cJSON_Number){
		if(lsddb_createPatchSpace(NULL,NULL,psId->valueint)<0){
			cJSON_AddStringToObject(resp,"error","Unable to add node instance to DB");
		}
		else{
			cJSON_AddStringToObject(resp,"success","success");
			cJSON_AddNumberToObject(resp,"psId",psId->valueint);
		}
    }
    else{
        cJSON_AddStringToObject(resp,"error","psId key not present or not a number");
    }
}

void lsdRemoveNode(cJSON* req, cJSON* resp){
    cJSON* nodeId = cJSON_GetObjectItem(req,"nodeId");
    if(nodeId && nodeId->type==cJSON_Number){
        
        if(lsddb_removeNodeInst(nodeId->valueint)<0){
            cJSON_AddStringToObject(resp,"error","Error while removing node");
        }
        else{
            cJSON_AddStringToObject(resp,"success","success");
        }
        
    }
    else{
        cJSON_AddStringToObject(resp,"error","nodeId key not present or not a number");
    }
}

void lsdWireNodes(cJSON* req, cJSON* resp){
    cJSON* leftFacadeInterior = cJSON_GetObjectItem(req,"leftFacadeInterior");
    if(!leftFacadeInterior || leftFacadeInterior->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","leftFacadeInterior not present or not a number");
        return;
    }
    
    cJSON* leftNodeId = cJSON_GetObjectItem(req,"leftNodeId");
    if(!leftNodeId || leftNodeId->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","leftNodeId not present or not a number");
        return;
    }
    
    cJSON* rightFacadeInterior = cJSON_GetObjectItem(req,"rightFacadeInterior");
    if(!rightFacadeInterior || rightFacadeInterior->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","rightFacadeInterior not present or not a number");
        return;
    }
    
    cJSON* rightNodeId = cJSON_GetObjectItem(req,"rightNodeId");
    if(!rightNodeId || rightNodeId->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","rightNodeId not present or not a number");
        return;
    }
    
    int wireId;
    if(lsddb_wireNodes(leftFacadeInterior->valueint,leftNodeId->valueint,rightFacadeInterior->valueint,rightNodeId->valueint,&wireId)<0){
        cJSON_AddStringToObject(resp,"error","Error while wiring nodes");
    }
    else{
        cJSON_AddStringToObject(resp,"success","success");
        cJSON_AddNumberToObject(resp,"wireId",wireId);
    }
}

void lsdUnwire(cJSON* req,cJSON* resp){
    cJSON* wireId = cJSON_GetObjectItem(req,"wireId");
    if(!wireId || wireId->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","wireId not present or not a number");
        return;
    }
    
    if(lsddb_unwireNodes(wireId->valueint)<0){
        cJSON_AddStringToObject(resp,"error","Error while unwiring nodes");
    }
    else{
        cJSON_AddStringToObject(resp,"success","success");
    }
}

void lsdPositionNode(cJSON* req, cJSON* resp){
    cJSON* nodeId = cJSON_GetObjectItem(req,"nodeId");
    if(!nodeId || nodeId->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","nodeId not present or not a number");
        return;
    }
    
    cJSON* xVal = cJSON_GetObjectItem(req,"x");
    if(!xVal || xVal->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","x not present or not a number");
        return;
    }
    
    cJSON* yVal = cJSON_GetObjectItem(req,"y");
    if(!yVal || yVal->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","y not present or not a number");
        return;
    }
    
    if(lsddb_nodeInstPos(nodeId->valueint,xVal->valueint,yVal->valueint)<0){
        cJSON_AddStringToObject(resp,"error","Error while positioning node");
    }
    else{
        cJSON_AddStringToObject(resp,"success","success");
    }
}

void lsdPositionFacade(cJSON* req, cJSON* resp){
    cJSON* nodeId = cJSON_GetObjectItem(req,"childPSId");
    if(!nodeId || nodeId->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","childPSId not present or not a number");
        return;
    }
    
    cJSON* xVal = cJSON_GetObjectItem(req,"x");
    if(!xVal || xVal->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","x not present or not a number");
        return;
    }
    
    cJSON* yVal = cJSON_GetObjectItem(req,"y");
    if(!yVal || yVal->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","y not present or not a number");
        return;
    }
    
    if(lsddb_facadeInstPos(nodeId->valueint,xVal->valueint,yVal->valueint)<0){
        cJSON_AddStringToObject(resp,"error","Error while positioning node");
    }
    else{
        cJSON_AddStringToObject(resp,"success","success");
    }
}

void lsdPanPatchSpace(cJSON* req, cJSON* resp){
	cJSON* psId = cJSON_GetObjectItem(req,"psId");
    if(!psId || psId->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","psId not present or not a number");
        return;
    }
    
    cJSON* xVal = cJSON_GetObjectItem(req,"x");
    if(!xVal || xVal->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","x not present or not a number");
        return;
    }
    
    cJSON* yVal = cJSON_GetObjectItem(req,"y");
    if(!yVal || yVal->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","y not present or not a number");
        return;
    }
	
	if(lsddb_panPatchSpace(psId->valueint,xVal->valueint,yVal->valueint)<0){
		cJSON_AddStringToObject(resp,"error","error");
		return;
	}
	
	cJSON_AddStringToObject(resp,"success","success");
}

void lsdGetChannelPatch(cJSON* req, cJSON* resp){
    if(lsddb_getPatchChannels(resp)<0){
        cJSON_AddStringToObject(resp,"error","Unable to get patch");
    }
}

void lsdCreatePartition(cJSON* req, cJSON* resp){
    cJSON* partName = cJSON_GetObjectItem(req,"name");
    if(!partName || partName->type!=cJSON_String){
        cJSON_AddStringToObject(resp,"error","Error while adding partition");
        return;
    }
    
    if(lsddb_createPartition(partName->valuestring,NULL)<0){
        cJSON_AddStringToObject(resp,"error","Unable to insert partition into DB");
        return;
    }
    
    cJSON_AddStringToObject(resp,"success","success");
}

void lsdUpdatePartition(cJSON* req, cJSON* resp){
    cJSON* partId = cJSON_GetObjectItem(req,"partId");
    if(!partId || partId->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","partId not valid");
        return;
    }
    
    cJSON* partName = cJSON_GetObjectItem(req,"name");
    if(!partName || partName->type!=cJSON_String){
        cJSON_AddStringToObject(resp,"error","name not valid");
        return;
    }
    
    if(lsddb_updatePartitionName(partId->valueint,partName->valuestring)<0){
        cJSON_AddStringToObject(resp,"error","Unable to update partition name");
        return;
    }
    
    cJSON_AddStringToObject(resp,"success","success");
}

void lsdDeletePartition(cJSON* req, cJSON* resp){
    cJSON* partId = cJSON_GetObjectItem(req,"partId");
    if(!partId || partId->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","partId not a valid value");
        return;
    }
    
    if(lsddb_removePartition(partId->valueint)<0){
        cJSON_AddStringToObject(resp,"error","Unable to remove partition from DB");
        return;
    }
    
    cJSON_AddStringToObject(resp,"success","success");
}

void lsdUpdateChannel(cJSON* req, cJSON* resp){
    cJSON* chanId = cJSON_GetObjectItem(req,"chanId");
    if(!chanId || chanId->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","chanId not a valid value");
        return;
    }
    
    if(lsddb_updatePatchChannel(chanId->valueint, req)<0)
        cJSON_AddStringToObject(resp,"error","error");
    else
        cJSON_AddStringToObject(resp,"success","success");
}

void lsdDeleteChannel(cJSON* req, cJSON* resp){
    cJSON* chanId = cJSON_GetObjectItem(req,"chanId");
    if(!chanId || chanId->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","chanId not a valid value");
        return;
    }
    
    if(lsddb_deletePatchChannel(chanId->valueint)<0)
        cJSON_AddStringToObject(resp,"error","error");
    else
        cJSON_AddStringToObject(resp,"success","success");
}

void lsdCreateChannel(cJSON* req, cJSON* resp){
    cJSON* partId = cJSON_GetObjectItem(req,"partId");
    if(!partId || partId->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","partId not a valid value");
        return;
    }
    
    if(lsddb_addPatchChannel(partId->valueint, req)<0)
        cJSON_AddStringToObject(resp,"error","error");
    else
        cJSON_AddStringToObject(resp,"success","success");
}

void lsdJsonPlugins(cJSON* req, cJSON* resp){
    if(lsddb_jsonPlugins(resp)<0)
        cJSON_AddStringToObject(resp,"error","unable to get plugins");
}

void lsdDisablePlugin(cJSON* req, cJSON* resp){
    cJSON* pId = cJSON_GetObjectItem(req,"pluginId");
    if(!pId || pId->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","pluginId not a valid value");
        return;
    }
    
    if(lsddb_disablePlugin(pId->valueint)<0)
        cJSON_AddStringToObject(resp,"error","Unable to disable plugin");
    else
        cJSON_AddStringToObject(resp,"success","success");
}

void lsdEnablePlugin(cJSON* req, cJSON* resp){
    cJSON* pId = cJSON_GetObjectItem(req,"pluginId");
    if(!pId || pId->type!=cJSON_Number){
        cJSON_AddStringToObject(resp,"error","pluginId not a valid value");
        return;
    }
    
    if(lsddb_enablePlugin(pId->valueint)<0)
        cJSON_AddStringToObject(resp,"error","Unable to enable plugin");
    else
        cJSON_AddStringToObject(resp,"success","success");
}



// Main request brancher
int handleJSONRequest(cJSON* req, cJSON* resp, int* reloadAfter){
    cJSON* method = cJSON_GetObjectItem(req,"method");
    if(method && method->type==cJSON_String){
        
        // Conditionally choose correct code path for method in question
        if(strcasecmp(method->valuestring,"lsdJsonLibrary")==0)
            lsdJsonLibrary(req,resp);
        else if(strcasecmp(method->valuestring,"lsdJsonPartitions")==0)
            lsdJsonPartitions(req,resp);
		/*
        else if(strcasecmp(method->valuestring,"lsdJsonNodes")==0)
            lsdJsonNodes(req,resp);
        else if(strcasecmp(method->valuestring,"lsdJsonWires")==0)
            lsdJsonWires(req,resp);
		 */
		else if(strcasecmp(method->valuestring,"lsdJsonPatchSpace")==0)
			lsdJsonPatchSpace(req,resp);
        else if(strcasecmp(method->valuestring,"lsdAddNode")==0)
            lsdAddNode(req,resp);
		else if(strcasecmp(method->valuestring,"lsdDeleteNode")==0)
            lsdDeleteNode(req,resp);
		else if(strcasecmp(method->valuestring,"lsdAddFacade")==0)
			lsdAddFacade(req,resp);
        else if(strcasecmp(method->valuestring,"lsdRemoveNode")==0)
            lsdRemoveNode(req,resp);
        else if(strcasecmp(method->valuestring,"lsdWireNodes")==0)
            lsdWireNodes(req,resp);
        else if(strcasecmp(method->valuestring,"lsdUnwire")==0)
            lsdUnwire(req,resp);
        else if(strcasecmp(method->valuestring,"lsdPositionNode")==0)
            lsdPositionNode(req,resp);
		else if(strcasecmp(method->valuestring,"lsdPositionFacade")==0)
            lsdPositionFacade(req,resp);
		else if(strcasecmp(method->valuestring,"lsdPanPatchSpace")==0)
			lsdPanPatchSpace(req,resp);
        else if(strcasecmp(method->valuestring,"lsdGetChannelPatch")==0)
            lsdGetChannelPatch(req,resp);
        else if(strcasecmp(method->valuestring,"lsdCreatePartition")==0){
            lsdCreatePartition(req,resp);
            *reloadAfter = 1;
        }
        else if(strcasecmp(method->valuestring,"lsdDeletePartition")==0){
            lsdDeletePartition(req,resp);
            *reloadAfter = 1;
        }
        else if(strcasecmp(method->valuestring,"lsdUpdatePartition")==0)
            lsdUpdatePartition(req,resp);
        else if(strcasecmp(method->valuestring,"lsdUpdateChannel")==0){
            lsdUpdateChannel(req,resp);
            *reloadAfter = 1;
        }
        else if(strcasecmp(method->valuestring,"lsdDeleteChannel")==0){
            lsdDeleteChannel(req,resp);
            *reloadAfter = 1;
        }
        else if(strcasecmp(method->valuestring,"lsdCreateChannel")==0){
            lsdCreateChannel(req,resp);
            *reloadAfter = 1;
        }
        else if(strcasecmp(method->valuestring,"lsdJsonPlugins")==0)
            lsdJsonPlugins(req,resp);
        else if(strcasecmp(method->valuestring,"lsdDisablePlugin")==0){
            lsdDisablePlugin(req,resp);
			*reloadAfter = 1;
		}
        else if(strcasecmp(method->valuestring,"lsdEnablePlugin")==0){
            lsdEnablePlugin(req,resp);
			*reloadAfter = 1;
		}
		else if(strcasecmp(method->valuestring,"lsdCustomRPC")==0)
			lsdCustomRPC(req,resp);

        else{
            cJSON_AddStringToObject(resp,"error","Specified method is not handled by this version of LSD");
        }
        
    }
    else{
        cJSON_AddStringToObject(resp,"error","Method key is not present or not a string");
        //return -1;
    }
    
    return 0;
}

// Callback for requests made to RPC
void rpcReqCB(struct evhttp_request* req, void* arg){
    // Flag to request a reload after request returns
    int reloadAfter = 0;
    
    struct evbuffer* inputPostBuf = evhttp_request_get_input_buffer(req);
	// Ensure input buffer is null terminated (buffer overflows are bad)
	evbuffer_add_printf(inputPostBuf,"%c",'\0');
	const unsigned char* inputPost = evbuffer_pullup(inputPostBuf,-1);
    //printf("%s\n",inputPost);
    
    // Setup json objects for parsing/returning
	cJSON* input = cJSON_Parse((const char*)inputPost);
    cJSON* returnjson = cJSON_CreateObject();
    
    if(input){
        if(handleJSONRequest(input,returnjson,&reloadAfter)<0){
            fprintf(stderr,"There was a problem while running RPC handler\n");
        }
    }
    else{
        cJSON_AddStringToObject(returnjson,"error","Unable to parse any JSON from HTTP POST data");
    }

	struct evbuffer* repBuf = evbuffer_new();
    
	// Print results into HTTP reply buffer
	char* returnjsonStr = cJSON_PrintUnformatted(returnjson);
    //printf("%s\n",returnjsonStr);
	evbuffer_add_printf(repBuf,"%s",returnjsonStr);
	free(returnjsonStr);
    
    // Memory leaks are bad... very bad
	cJSON_Delete(returnjson);
	cJSON_Delete(input);
    
	evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
	evhttp_send_reply(req,200,"OK",repBuf);
	
	evbuffer_free(repBuf);
    
    if(reloadAfter)
        handleReload(0,0,NULL);
}

// Catchall for requests not made to /lsd/rpc
void reqCB(struct evhttp_request* req, void* arg){
    struct evbuffer* repBuf = evbuffer_new();
	evbuffer_add_printf(repBuf,"<html><body>");
	evbuffer_add_printf(repBuf,"<h1>I don't handle %s</h1>", req->uri);
	evbuffer_add_printf(repBuf,"<p>Version: %s</p>",event_get_version());
	evbuffer_add_printf(repBuf,"</body></html>");
	evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/html");
	evhttp_send_reply(req,200,"OK",repBuf);
	evbuffer_free(repBuf);
}

void srvIndexCB(struct evhttp_request* req, void* arg){
    struct evbuffer* repBuf = evbuffer_new();
    if(lsddb_indexHtmlGen("Plugins",repBuf)<0){
        evbuffer_add_printf(repBuf,"Error while generating index\n");
    }
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/html");
	evhttp_add_header(evhttp_request_get_output_headers(req), "Pragma", "no-cache");
    evhttp_send_reply(req,200,"OK",repBuf);
    evbuffer_free(repBuf);
}

// Libevent stuff below

static struct event_base* eb;
static struct evhttp* eh;

int openRPC(struct event_base* ebin, int port){
    eb = ebin;
	eh = evhttp_new(eb);
    if(evhttp_bind_socket(eh, "0.0.0.0", port)<0){
        return -1;
    }
	evhttp_set_gencb(eh,reqCB,NULL);
	evhttp_set_cb(eh,"/lsdnew/main/rpc",rpcReqCB,NULL);
    evhttp_set_cb(eh,"/lsdnew/main/",srvIndexCB,NULL);
    
    return 0;
}

int closeRPC(){
    evhttp_free(eh);
    
    return 0;
}
