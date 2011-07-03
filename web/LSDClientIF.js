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


/* LSD Server Interface Prototype - Depends on JQuery */


function LSDServer(url){
    this.url = url;
}

LSDServer.prototype = {
    getLibrary:function(resultCB){
        $.post(this.url,JSON.stringify({method:"lsdJsonLibrary"}),resultCB,"json");
    },
    
    getPartitions:function(resultCB){
        $.post(this.url,JSON.stringify({method:"lsdJsonPartitions"}),resultCB,"json");
    },

    getPatchSpace:function(psId,resultCB){
        $.post(this.url,JSON.stringify({method:"lsdJsonPatchSpace",psId:psId}),resultCB,"json");
    },
    
    addNode:function(psId,classId,x,y,resultCB){
        $.post(this.url,JSON.stringify({method:"lsdAddNode",psId:psId,classId:classId,x:x,y:y}),resultCB,"json");
    },
    
    deleteNode:function(nodeId,resultCB){
        $.post(this.url,JSON.stringify({method:"lsdDeleteNode",nodeId:nodeId}),resultCB,"json");
    },
    
    updateNodeName:function(nodeId,name,resultCB){
        $.post(this.url,JSON.stringify({method:"lsdUpdateNodeName",nodeId:nodeId,name:name}),resultCB,"json");
    },
    
    setNodeColour:function(nodeId,colour,resultCB){
        $.post(this.url,JSON.stringify({method:"lsdSetNodeColour",nodeId:nodeId,colour:colour}),resultCB,"json");
    },
    
    addFacade:function(psId,x,y,resultCB){
        $.post(this.url,JSON.stringify({method:"lsdAddFacade",psId:psId,x:x,y:y}),resultCB,"json");
    },
    
    deleteFacade:function(facNodeId,resultCB){
        $.post(this.url,JSON.stringify({method:"lsdDeleteFacade",facNodeId:facNodeId}),resultCB,"json");
    },
    
    updateFacadeName:function(facNodeId,name,resultCB){
        $.post(this.url,JSON.stringify({method:"lsdUpdateFacadeName",facNodeId:facNodeId,name:name}),resultCB,"json");
    },
    
    setFacadeColour:function(facNodeId,colour,resultCB){
        $.post(this.url,JSON.stringify({method:"lsdSetFacadeColour",facNodeId:facNodeId,colour:colour}),resultCB,"json");
    },
    
    getFacade:function(facNodeId,resultCB){
        $.post(this.url,JSON.stringify({method:"lsdGetFacade",facNodeId:facNodeId}),resultCB,"json");
    },
        
    createFacadeIn:function(name,facNodeId,resultCB){
        $.post(this.url,JSON.stringify({method:"lsdCreateFacadeIn",facNodeId:facNodeId,name:name}),resultCB,"json");
    },
        
    deleteFacadeIn:function(inId,resultCB){
        $.post(this.url,JSON.stringify({method:"lsdDeleteFacadeIn",inId:inId}),resultCB,"json");
    },
        
    updateFacadeIn:function(inId,name,resultCB){
        $.post(this.url,JSON.stringify({method:"lsdUpdateFacadeIn",inId:inId,name:name}),resultCB,"json");
    },
        
    createFacadeOut:function(name,facNodeId,resultCB){
        $.post(this.url,JSON.stringify({method:"lsdCreateFacadeOut",facNodeId:facNodeId,name:name}),resultCB,"json");
    },
        
    deleteFacadeOut:function(outId,resultCB){
        $.post(this.url,JSON.stringify({method:"lsdDeleteFacadeOut",outId:outId}),resultCB,"json");
    },
        
    updateFacadeOut:function(outId,name,resultCB){
        $.post(this.url,JSON.stringify({method:"lsdUpdateFacadeOut",outId:outId,name:name}),resultCB,"json");
    },
    
    positionNode:function(nodeId,x,y){
        $.post(this.url,JSON.stringify({method:"lsdPositionNode",nodeId:nodeId,x:x,y:y}),null,"json");
    },
    
    positionFacade:function(childPSId,x,y){
        $.post(this.url,JSON.stringify({method:"lsdPositionFacade",childPSId:childPSId,x:x,y:y}),null,"json");
    },
    
    panPatchSpace:function(psId,x,y,scale){
        $.post(this.url,JSON.stringify({method:"lsdPanPatchSpace",psId:psId,x:x,y:y,scale:scale}),null,"json");
    },
    
    wireNodes:function(leftFacadeInterior,leftNodeId,rightFacadeInterior,rightNodeId,resultCB){
        $.post(this.url,JSON.stringify({method:"lsdWireNodes",leftFacadeInterior:leftFacadeInterior,leftNodeId:leftNodeId,rightFacadeInterior:rightFacadeInterior,rightNodeId:rightNodeId}),resultCB,"json");
        //alert(JSON.stringify({method:"lsdWireNodes",leftFacadeInterior:leftFacadeInterior,leftNodeId:leftNodeId,rightFacadeInterior:rightFacadeInterior,rightNodeId:rightNodeId}));
    },
    
    unwireNodes:function(wireId,resultCB){
        $.post(this.url,JSON.stringify({method:"lsdUnwire",wireId:wireId}),resultCB,"json");
    },
    
    getChannelPatch:function(resultCB){
        $.post(this.url,JSON.stringify({method:"lsdGetChannelPatch"}),resultCB,"json");
    },
    
    updatePartition:function(partId,name,imageFile){
        var postObj = {method:"lsdUpdatePartition",partId:partId,name:name};
        if(imageFile)
            postObj.imageFile = imageFile;
        $.post(this.url,JSON.stringify(postObj),null,"json");
    },
    
    deletePartition:function(partId){
        $.post(this.url,JSON.stringify({method:"lsdDeletePartition",partId:partId}),null,"json");
    },
    
    createPartition:function(name,doneCB,imageFile){
        var postObj = {method:"lsdCreatePartition",name:name};
        if(imageFile)
            postObj.imageFile = imageFile;
        $.post(this.url,JSON.stringify(postObj),doneCB,"json");
    },
    
    updateChannel:function(chanId,name,single,sixteenBit,redAddr,greenAddr,blueAddr){
        $.post(this.url,JSON.stringify({method:"lsdUpdateChannel",chanId:chanId,name:name,single:single,sixteenBit:sixteenBit,redAddr:redAddr,greenAddr:greenAddr,blueAddr:blueAddr}),null,"json");
        //alert(JSON.stringify({method:"lsdUpdateChannel",chanId:chanId,name:name,single:single,sixteenBit:sixteenBit,redAddr:redAddr,greenAddr:greenAddr,blueAddr:blueAddr}));
    },
    
    deleteChannel:function(chanId){
        $.post(this.url,JSON.stringify({method:"lsdDeleteChannel",chanId:chanId}),null,"json");
        //alert(JSON.stringify({method:"lsdDeleteChannel",chanId:chanId}));
    },
    
    createChannel:function(partId,name,single,sixteenBit,redAddr,greenAddr,blueAddr,doneCB){
        $.post(this.url,JSON.stringify({method:"lsdCreateChannel",partId:partId,name:name,single:single,sixteenBit:sixteenBit,redAddr:redAddr,greenAddr:greenAddr,blueAddr:blueAddr}),doneCB,"json");
        //alert(JSON.stringify({method:"lsdCreateChannel",partId:partId,name:name,single:single,sixteenBit:sixteenBit,redAddr:redAddr,greenAddr:greenAddr,blueAddr:blueAddr}));
    },
    
    getNodeConfFunc:function(nodeId,resultCB){
        
    },
    
    getPlugins:function(resultCB){
        $.post(this.url,JSON.stringify({method:"lsdJsonPlugins"}),resultCB,"json");
    },
    
    enablePlugin:function(pluginId){
        $.post(this.url,JSON.stringify({method:"lsdEnablePlugin",pluginId:pluginId}),null,"json");
    },
    
    disablePlugin:function(pluginId){
        $.post(this.url,JSON.stringify({method:"lsdDisablePlugin",pluginId:pluginId}),null,"json");
    },

    customRPC:function(nodeId,data,resultCB){
        if(!nodeId)
            return;
        
        var thedata = data;
        thedata.method = "lsdCustomRPC";
        thedata.nodeId = nodeId;
        $.post(this.url,JSON.stringify(thedata),resultCB,"json");
    }
};
