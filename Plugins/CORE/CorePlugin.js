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

var CORE_theIntGen;
function CORE_IntGenerator(nodeId,server){
    CORE_theIntGen = this;
    this.server = server;
    this.nodeId = nodeId;
    
    this.dialog = $(document.createElement('div'));
    this.dialog.dialog({title:"Integer Gen", width:200, height:100, modal:true, resizable:false});
    this.dialog.bind('dialogclose',this,function(event){
                        var val = parseInt(event.data.numberBox.val());
                        if(!isNaN(val)){
                            event.data.server.customRPC(event.data.nodeId,{coreMethod:"setIntGenVal",val:val});
                        }
                     });
    
    this.numberBox = $(document.createElement('input'));
    this.numberBox.addClass('CORE_numBox');
    this.dialog.append('<span class="CORE_numLabel">Value:</span>');
    this.dialog.append(this.numberBox);
    
    this.server.customRPC(this.nodeId,{coreMethod:"getIntGenVal"},function(data){
                          CORE_theIntGen.numberBox.val(data.val);});
}

var CORE_theIntViewer;
function CORE_IntViewer(nodeId,server){
    CORE_theIntViewer = this;
    this.server = server;
    this.nodeId = nodeId;
    
    this.dialog = $(document.createElement('div'));
    this.dialog.dialog({title:"Integer View", width:200, height:100, modal:true, resizable:false});
    
    this.numberBox = $(document.createElement('input'));
    this.numberBox.addClass('CORE_numBox');
    this.numberBox.attr('readonly','readonly');
    this.dialog.append('<span class="CORE_numLabel">Value:</span>');
    this.dialog.append(this.numberBox);
    
    this.server.customRPC(this.nodeId,{coreMethod:"getIntViewVal"},function(data){
                          CORE_theIntViewer.numberBox.val(data.val);});
}

var CORE_theFloatGen;
function CORE_FloatGenerator(nodeId,server){
    CORE_theFloatGen = this;
    this.server = server;
    this.nodeId = nodeId;
    
    this.dialog = $(document.createElement('div'));
    this.dialog.dialog({title:"Float Gen", width:200, height:100, modal:true, resizable:false});
    this.dialog.bind('dialogclose',this,function(event){
                     var val = parseFloat(event.data.numberBox.val());
                     if(!isNaN(val)){
                     event.data.server.customRPC(event.data.nodeId,{coreMethod:"setFloatGenVal",val:val});
                     }
                     });
    
    this.numberBox = $(document.createElement('input'));
    this.numberBox.addClass('CORE_numBox');
    this.dialog.append('<span class="CORE_numLabel">Value:</span>');
    this.dialog.append(this.numberBox);
    
    this.server.customRPC(this.nodeId,{coreMethod:"getFloatGenVal"},function(data){
                          CORE_theFloatGen.numberBox.val(data.val);});
} 

var CORE_theFloatViewer;
function CORE_FloatViewer(nodeId,server){
    CORE_theFloatViewer = this;
    this.server = server;
    this.nodeId = nodeId;
    
    this.dialog = $(document.createElement('div'));
    this.dialog.dialog({title:"Float View", width:200, height:100, modal:true, resizable:false});
    
    this.numberBox = $(document.createElement('input'));
    this.numberBox.addClass('CORE_numBox');
    this.numberBox.attr('readonly','readonly');
    this.dialog.append('<span class="CORE_numLabel">Value:</span>');
    this.dialog.append(this.numberBox);
    
    this.server.customRPC(this.nodeId,{coreMethod:"getFloatViewVal"},function(data){
                          CORE_theFloatViewer.numberBox.val(data.val);});
} 

var CORE_theRgbGen;
function CORE_RGBGenerator(nodeId,server){
    CORE_theRgbGen = this;
    this.nodeId = nodeId;
    this.server = server;
    
    this.dialog = $(document.createElement('div'));
    this.dialog.dialog({title:"RGB Gen",width:230,height:250,modal:true,resizable:false});
    this.dialog.bind('dialogclose',this.dialog,function(event){event.data.remove();});
    
    var newFTdiv = $(document.createElement('div'));
    newFTdiv.css('float','left');
    this.dialog.append(newFTdiv);
    this.newFT = $.farbtastic(newFTdiv,function(picker){
                             var dia = CORE_theRgbGen;
                             dia.server.customRPC(dia.nodeId,{coreMethod:"setRgbGenVal",val:{r:picker.rgb[0],
                                                  g:picker.rgb[1],b:picker.rgb[2]}});
                             });
    this.server.customRPC(this.nodeId,{coreMethod:"getRgbGenVal"},function(data){
                          var dia = CORE_theRgbGen;
                          dia.newFT.setRGB([data.val.r,data.val.g,data.val.b]);
                          });
}  

var CORE_theRgbViewer;
function CORE_RGBViewer(nodeId,server){
    CORE_theRgbViewer = this;
    
    this.nodeId = nodeId;
    this.server = server;
    
    this.dialog = $(document.createElement('div'));
    this.dialog.dialog({title:"RGB View",width:200,height:200,modal:true,resizable:false});
    this.dialog.bind('dialogclose',this.dialog,function(event){event.data.remove();});
    
    this.viewbox = $(document.createElement('div'));
    this.viewbox.css('float','left').css('position','absolute');
    this.viewbox.css('left','1em').css('top','1em');
    this.viewbox.css('width','9.3em').css('height','7.5em');
    this.dialog.append(this.viewbox);
    
    this.server.customRPC(this.nodeId,{coreMethod:"getRgbViewVal"},function(data){
                          var dia = CORE_theRgbViewer;
                          dia.viewbox.css('background-color','rgba('+parseInt(data.val.r*255)+
                                          ','+parseInt(data.val.g*255)+
                                          ','+parseInt(data.val.b*255)+',1)');
                          });
} 

var CORE_theTriggerGen;
function CORE_TriggerGenerator(nodeId,server){
    CORE_theTriggerGen = this;
    
    this.nodeId = nodeId;
    this.server = server;
    
    this.dialog = $(document.createElement('div'));
    this.dialog.dialog({title:"Trigger Gen",width:200,height:100,modal:true,resizable:false});
    
    var button = $(document.createElement('button'));
    this.dialog.append(button);
    button.button({label:"Trigger"});
    button.click(this,function(event){event.data.server.customRPC(event.data.nodeId,{coreMethod:"triggerGen"});});
}


function CORE_Conf(classIdx,nodeId,server){
    if(classIdx == 1)
        new CORE_IntGenerator(nodeId,server);
    else if(classIdx == 2)
        new CORE_IntViewer(nodeId,server);
    else if(classIdx == 3)
        new CORE_FloatGenerator(nodeId,server);
    else if(classIdx == 4)
        new CORE_FloatViewer(nodeId,server);
    else if(classIdx == 5)
        new CORE_RGBGenerator(nodeId,server);
    else if(classIdx == 6)
        new CORE_RGBViewer(nodeId,server);
    else if(classIdx == 7)
        new CORE_TriggerGenerator(nodeId,server);
}

CORE_CoreHead = {confFunc:CORE_Conf};
