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
var ColourBankClient_Dialog;


function LSDColourBankDialog(nodeId,server){
	this.nodeId = nodeId;
	this.server = server;
	this.dialog = $(document.createElement('div')).dialog({modal:true, width:1020, height:250, title:'Colour Bank', resizable:false, close:function(){lsdApp.reloadCurView()}});
	
	// Picker Div
	this.pickerDiv = $(document.createElement('div'));
	this.pickerDiv.css('float','left');
	this.dialog.append(this.pickerDiv);
	
	this.lastPicker = null;
	
	// Add Button
    this.addButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-plus'}}).click(this,this.addClick);
    // Remove button
    this.removeButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-minus'}}).click(this,this.removeClick);
    var buttonDiv = $(document.createElement('div')).append(this.addButton).append(this.removeButton).css('position','absolute').css('right','1em').css('bottom','1em');
	
	this.dialog.append(buttonDiv);
	
	this.addButton.button('disable');
	this.removeButton.button('disable');
	
	this.getPickers();
}


LSDColourBankDialog.prototype = {
	getPickers:function(){
		this.server.customRPC(this.nodeId,{cbMethod:'getNodePickers'},this.handlePickerResp);
	},
	
	getPickersWrap:function(){
		ColourBankClient_Dialog.getPickers();
	},
	
	handlePickerResp:function(data){
		var dia = ColourBankClient_Dialog;
		
		dia.pickerDiv.empty();
		
		for(var i in data.pickers){
			// Create new farbtastic for each picker
			var newFTdiv = $(document.createElement('div'));
			newFTdiv.css('float','left');
			var newFT = $.farbtastic(newFTdiv,dia.handlePickerChange);
			newFT.setData(data.pickers[i].pickerId);
			newFT.setRGB([data.pickers[i].rgb.r,data.pickers[i].rgb.g,data.pickers[i].rgb.b]);
			dia.pickerDiv.append(newFTdiv);
			dia.lastPicker = newFT;
		}
		
		if(dia.lastPicker)
			dia.removeButton.button('enable');
		dia.addButton.button('enable');
	},
	
	handlePickerChange:function(picker){
		var dia = ColourBankClient_Dialog;
		//alert(picker.getData());
		dia.server.customRPC(dia.nodeId,{cbMethod:'updatePicker',pickerId:picker.getData(),
							 r:picker.rgb[0],g:picker.rgb[1],b:picker.rgb[2]});
	},
	
	addClick:function(event){
		event.data.server.customRPC(event.data.nodeId,{cbMethod:'addPicker'},event.data.getPickersWrap);
	},
	
	removeClick:function(event){
		event.data.server.customRPC(event.data.nodeId,{cbMethod:'deletePicker',
									pickerId:event.data.lastPicker.getData()},event.data.getPickersWrap);
	}
};


function FaderPlugin_confFunc(classId,nodeId,server){	
	ColourBankClient_Dialog = new LSDColourBankDialog(nodeId,server);
}


ColourBankPlugin_CoreHead = {confFunc:FaderPlugin_confFunc};

