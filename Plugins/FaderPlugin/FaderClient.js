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

// Currently open fader dialog, used to resolve dialog after RPC responses
var FaderClient_faderDialog;


function LSDFaderPluginDialog(nodeId,server){
	this.nodeId = nodeId;
	this.server = server;
	this.dialog = $(document.createElement('div')).dialog({modal:true, width:1020, height:250, title:'Color Bank', resizable:false});
	
	this.faderDataArr = new Array();
	
	
	for(var i=0;i<5;++i){
		var cpPlac = $(document.createElement('div')).css('float','left');
		this.faderDataArr.push($.farbtastic(cpPlac,this.handleRGBChange));
		this.dialog.append(cpPlac);
	}
	
	
	this.getState();
}


LSDFaderPluginDialog.prototype = {
getState:function(){
	this.server.customRPC(this.nodeId,{faderMethod:'getState'},this.handleStateResp)
},
	
handleStateResp:function(data){
	//FaderClient_faderDialog.dialog.append('<p>'+JSON.stringify(data)+'</p>');
	
	for(var i in data.faders){
		FaderClient_faderDialog.faderDataArr[i].setRGB([data.faders[i].rgb.r,data.faders[i].rgb.g,data.faders[i].rgb.b]);
	}
},
	
	
handleRGBChange:function(fader){
	var faderArr = new Array();
	
	for(var i in FaderClient_faderDialog.faderDataArr){
		var cpRgb = FaderClient_faderDialog.faderDataArr[i].rgb;
		faderArr.push({intensity:1,rgb:{r:cpRgb[0],g:cpRgb[1],b:cpRgb[2]}});
	}
	
	FaderClient_faderDialog.server.customRPC(FaderClient_faderDialog.nodeId,{faderMethod:'setState',faders:faderArr});
}
};


function FaderPlugin_confFunc(classId,nodeId,server){
	//alert('Fader Plugin Configuring '+classId+','+nodeId+'!');
	
	FaderClient_faderDialog = new LSDFaderPluginDialog(nodeId,server);

}


FaderPlugin_CoreHead = {confFunc:FaderPlugin_confFunc};

