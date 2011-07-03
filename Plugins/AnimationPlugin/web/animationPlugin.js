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

AnimationPlugin = {};
AnimationPlugin.theAdConf = null;
AnimationPlugin.adConf = function(nodeId,server){
    AnimationPlugin.theAdConf = this;
    this.server = server;
    this.nodeId = nodeId;
    
    this.dialog = $(document.createElement('div'));
    this.dialog.dialog({title:"Attack/Decay Conf", width:230, height:140, modal:true, resizable:false});
    this.dialog.bind('dialogclose',this,function(event){
                     var attackVal = parseFloat(event.data.attackBox.val());
                     var decayVal = parseFloat(event.data.decayBox.val());

                     if(!isNaN(attackVal)){
                     event.data.server.customRPC(event.data.nodeId,{animationMethod:"updateAttack",attackAmt:attackVal});
                     }
                     
                     if(!isNaN(decayVal)){
                     event.data.server.customRPC(event.data.nodeId,{animationMethod:"updateDecay",decayAmt:decayVal});
                     }
                     
                     event.data.dialog.remove();
                     });
    
    var attackDiv = $(document.createElement('div')).addClass('AnimationPlugin_valDiv').css('margin-top','0.5em');
    this.dialog.append(attackDiv);
    
    this.attackBox = $(document.createElement('input'));
    this.attackBox.addClass('AnimationPlugin_numBox');
    attackDiv.append('<span class="AnimationPlugin_numLabel">Attack:</span>');
    attackDiv.append(this.attackBox);
    
    var decayDiv = $(document.createElement('div')).addClass('AnimationPlugin_valDiv');
    this.dialog.append(decayDiv);
    
    this.decayBox = $(document.createElement('input'));
    this.decayBox.addClass('AnimationPlugin_numBox');
    decayDiv.append('<span class="AnimationPlugin_numLabel">Decay:</span>');
    decayDiv.append(this.decayBox);
    
    this.server.customRPC(this.nodeId,{animationMethod:"getAttackDecay"},function(data){
                          AnimationPlugin.theAdConf.attackBox.val(data.attackVal);
                          AnimationPlugin.theAdConf.decayBox.val(data.decayVal);});
};



AnimationPlugin.confFunc = function(classIdx,nodeId,server){
    if(classIdx == 0)
        new AnimationPlugin.adConf(nodeId,server);
};


AnimationPlugin_CoreHead = {confFunc:AnimationPlugin.confFunc};
