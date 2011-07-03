
// Gradient generator
function GenerateGradientStrip(colourArr){
    
    var strip = $(document.createElement('div'));
    
    var grad = document.createElementNS('http://www.w3.org/2000/svg','svg');
    grad.setAttribute('version','1.1');
    grad.setAttribute('width','100%');
    grad.setAttribute('height','100%');
    grad.setAttribute('xmlns','http://www.w3.org/2000/svg');
    
    strip.append(grad);
    
    var rect = document.createElementNS('http://www.w3.org/2000/svg','rect');
    rect.setAttribute('width','100%');
    rect.setAttribute('height','100%');
    grad.appendChild(rect);
    
    if(colourArr.length <= 1){
        if(colourArr.length == 1)
            rect.setAttribute('style','fill:rgba('+parseInt(255*colourArr[0].r)+','+parseInt(255*colourArr[0].g)+','+
                              parseInt(255*colourArr[0].b)+',1);');
        else
            rect.setAttribute('style','fill:rgba(0,0,0,1);');
    }
    else{
    
        var linGrad = document.createElementNS('http://www.w3.org/2000/svg','linearGradient');
        linGrad.setAttribute('id','sampleStripGrad');
        grad.appendChild(linGrad);
        rect.setAttribute('style','fill:url(#sampleStripGrad);');
        
        var interval = 100 / (colourArr.length - 1);
        var curOffset = 0;
        
        for(var i in colourArr){
            var stop = document.createElementNS('http://www.w3.org/2000/svg','stop');
            stop.setAttribute('stop-color','rgba('+parseInt(255*colourArr[i].r)+','+parseInt(255*colourArr[i].g)+','+
                              parseInt(255*colourArr[i].b)+',1)');
            stop.setAttribute('offset',curOffset+'%');
            linGrad.appendChild(stop);
            curOffset += interval;
        }
        
    }
    
    return strip;
}



// Gradient object with sample stops
function GradientSamplerStrip(colourArr,sampleStopArr,server,nodeId){
    
    var strip = $(document.createElement('div'));
    strip.width(465);
    
    var gradStrip = GenerateGradientStrip(colourArr);
    gradStrip.height(25);
    gradStrip.width(449);
    gradStrip.css('position','relative').css('left','8px');
    strip.append(gradStrip);
    
    var sliderStrip = $(document.createElement('div'));
    sliderStrip.width(465);
    sliderStrip.css('position','relative').css('clear','both');
    strip.append(sliderStrip);
    
    for(var i in sampleStopArr){
        var stop = $(document.createElement('div'));
        stop.addClass('PaletteMarker');
        stop.data({stopId:sampleStopArr[i].stopId,server:server,nodeId:nodeId});
        stop.append(sampleStopArr[i].stopIdx+1);
        stop.draggable({axis:'x',containment:'parent'});
        stop.css('left',parseInt(sampleStopArr[i].pos*415)+'px');
        sliderStrip.append(stop);
        stop.bind('drag',stop,handleSamplerDrag);
    }
    
    return strip;
}

function handleSamplerDrag(event,ui){
    var stopData = event.data.data();
    var pos = event.data.position().left / 449;
    stopData.server.customRPC(stopData.nodeId,{paletteMethod:"setSamplePos",outId:stopData.stopId,pos:pos});
}

var thePaletteDBEditor = null;

// Palette DB editor
function PaletteDBEditor(paletteArr,server,parent){
    this.server = server;
    this.parent = parent;
    
    // List of palettes
    this.paletteUl = $(document.createElement('ul')).selectable();
    this.paletteUl.addClass('dialogList').css('width','100%');
    this.paletteUl.bind('selectablestop',this,this.procPalSel);
    this.paletteUl.dblclick(this,this.palEditClick);
    
    // Add/remove palette buttons
    this.palAddButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-plus'}});
    this.palRemoveButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-minus'}});
    this.palActivateButton = $(document.createElement('button')).button({label:'Use Palette'});
    this.palActivateButton.css('width','8em').css('height','2.2em').css('padding','0').css('font-size','0.5em');
    this.palActivateButton.css('top','0.3em').css('left','0.3em');
    this.palEditButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-pencil'}});
    this.palEditButton.css('right','0px').css('position','absolute');
    var palManipDiv = $(document.createElement('div'));
    palManipDiv.addClass('listManipButtons').css('height','1px');
    palManipDiv.append(this.palAddButton);
    palManipDiv.append(this.palRemoveButton);
    palManipDiv.append(this.palActivateButton);
    palManipDiv.append(this.palEditButton);
    
    this.palAddButton.click(this,this.palAddClick);
    this.palRemoveButton.click(this,this.palRemoveClick);
    this.palActivateButton.click(this,this.palActivateClick);
    this.palEditButton.click(this,this.palEditClick);

    
    
    // List of swatches
    this.swatchUl = $(document.createElement('ul')).selectable();
    this.swatchUl.addClass('dialogList').addClass('Palette_SwatchList').css('width','100%');
    this.swatchUl.bind('selectablestop',this,this.procSwaSel);
    this.swatchUl.dblclick(this,this.swaEditClick);
    
    
    // Add/remove swatch buttons
    this.swaAddButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-plus'}});
    this.swaRemoveButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-minus'}});
    this.swaEditButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-pencil'}});
    this.swaEditButton.css('right','0px').css('position','absolute');
    var swaManipDiv = $(document.createElement('div'));
    swaManipDiv.addClass('listManipButtons');
    swaManipDiv.append(this.swaAddButton);
    swaManipDiv.append(this.swaRemoveButton);
    swaManipDiv.append(this.swaEditButton);
    
    this.swaAddButton.click(this,this.swaAddClick);
    this.swaRemoveButton.click(this,this.swaRemoveClick);
    this.swaEditButton.click(this,this.swaEditClick);
    
    
    // Main div
    var palEditor = $(document.createElement('div'));
    palEditor.addClass('PaletteEditor').css('clear','both');
    
    palEditor.append('<h3>Palettes:</h3>');
    palEditor.append(this.paletteUl);
    palEditor.append(palManipDiv);
    palEditor.append('<h3>Swatches:</h3>');
    palEditor.append(this.swatchUl);
    palEditor.append(swaManipDiv);
    
    // Add and Process editor members
    for(var i in paletteArr){
        var item = $(document.createElement('li'));
        item.addClass('ui-widget-content');
        item.data(paletteArr[i]);
        item.append(paletteArr[i].paletteId + ' - ' + paletteArr[i].paletteName);
        this.paletteUl.append(item);
        if(i==this.parent.lastSelPal)
            item.addClass('ui-selected');
    }
    
    this.procPalSel({data:this});
    
    return palEditor;
    
}

PaletteDBEditor.prototype = {
    procPalSel:function(event){
        var selected = $('.ui-selected',event.data.paletteUl);
        var selSize = selected.size();
        
        if(selSize == 0){
            event.data.parent.lastSelPal = null;
            event.data.palRemoveButton.button('disable');
            event.data.palActivateButton.button('disable');
            event.data.palEditButton.button('disable');
            event.data.swatchUl.selectable('disable');
            event.data.swatchUl.empty();
            event.data.swaAddButton.button('disable');
            event.data.swaRemoveButton.button('disable');
            event.data.swaEditButton.button('disable');
        }
        else if(selSize > 1){
            event.data.parent.lastSelPal = null;
            event.data.palRemoveButton.button('enable');
            event.data.palActivateButton.button('disable');
            event.data.palEditButton.button('disable');
            event.data.swatchUl.selectable('disable');
            event.data.swatchUl.empty();
            event.data.swaAddButton.button('disable');
            event.data.swaRemoveButton.button('disable');
            event.data.swaEditButton.button('disable');
        }
        else{
            event.data.parent.lastSelPal = selected.index();
            event.data.palRemoveButton.button('enable');
            event.data.palActivateButton.button('enable');
            event.data.palEditButton.button('enable');
            event.data.swatchUl.selectable('enable');			
            event.data.swaAddButton.button('disable');
            event.data.swaRemoveButton.button('disable');
            event.data.swaEditButton.button('disable');
            
            // Populate swatch list
            var item = selected.first();
            var itemSwatches = item.data().paletteSwatches;
            
            event.data.swatchUl.empty();
            for(var i in itemSwatches){
                var swa = $(document.createElement('li'));
                swa.data(itemSwatches[i]);
                swa.addClass('ui-widget-content');
                swa.css('background-image','none');
                swa.css('background-color','rgba('+parseInt(255*itemSwatches[i].rVal)+','+
                        parseInt(255*itemSwatches[i].gVal)+','+parseInt(255*itemSwatches[i].bVal)+',1)');
                swa.height(20);
                event.data.swatchUl.append(swa);
            }
            
            event.data.procSwaSel(event);
            
        }
        
    },
    
    procSwaSel:function(event){
        var selected = $('.ui-selected',event.data.swatchUl);
        var selSize = selected.size();
        
        if(selSize == 0){
            event.data.swaAddButton.button('enable');
            event.data.swaRemoveButton.button('disable');
            event.data.swaEditButton.button('disable');
        }
        else if(selSize > 1){
            event.data.swaAddButton.button('enable');
            event.data.swaRemoveButton.button('enable');
            event.data.swaEditButton.button('disable');
        }
        else{
            event.data.swaAddButton.button('enable');
            event.data.swaRemoveButton.button('enable');
            event.data.swaEditButton.button('enable');
        }
    },
    
    palAddClick:function(event){
        new LSDDialogueName("Add Palette","Palette Name:",event.data,event.data.palAddFollow);
    },
    palAddFollow:function(data,name){
        thePaletteDBEditor = data;
        data.server.customRPC(data.parent.nodeId,{paletteMethod:"insertPalette",name:name},data.updateFromServer);
    },
    
    palRemoveClick:function(event){
        thePaletteDBEditor = event.data;
        var selected = $('.ui-selected',event.data.paletteUl);
        selected.each(function(){thePaletteDBEditor.server.customRPC(thePaletteDBEditor.parent.nodeId,{paletteMethod:"deletePalette",paletteId:$(this).data().paletteId});$(this).remove();});

        event.data.procPalSel({data:event.data});
    },
    
    palActivateClick:function(event){
        thePaletteDBEditor = event.data;
        var selectedpal = $('.ui-selected',event.data.paletteUl);
        var palitem = selectedpal.first();
        var obj = {paletteMethod:"activatePalette",paletteId:palitem.data().paletteId};
        event.data.server.customRPC(event.data.parent.nodeId,obj,thePaletteDBEditor.updateFromServer);
    },
    
    palEditClick:function(event){
        thePaletteDBEditor = event.data;
        var selected = $('.ui-selected',event.data.paletteUl);
        var item = selected.first();
        new LSDDialogueName("Rename Palette","Palette Name:",item.data().paletteId,
                            event.data.palEditFollow,item.data().paletteName);
    },
    palEditFollow:function(data,name){
        thePaletteDBEditor.server.customRPC(thePaletteDBEditor.parent.nodeId,
                                            {paletteMethod:"updatePalette",name:name,paletteId:data},
                                            thePaletteDBEditor.updateFromServer);
    },
    
    swaAddClick:function(event){
        thePaletteDBEditor = event.data;
        var selectedpal = $('.ui-selected',event.data.paletteUl);
        var palitem = selectedpal.first();
        
        new LSDDialogueNodeColour("Select Swatch Colour",{r:0,g:0,b:0},palitem.data().paletteId,event.data.swaAddFollow);
    },
    swaAddFollow:function(data,colour){
        thePaletteDBEditor.server.customRPC(thePaletteDBEditor.parent.nodeId,
                                            {paletteMethod:"insertSwatch",
                                            paletteId:data,colour:colour},
                                            thePaletteDBEditor.updateFromServer);
    },
    
    swaRemoveClick:function(event){
        thePaletteDBEditor = event.data;
        var selected = $('.ui-selected',event.data.swatchUl);
        selected.each(function(){thePaletteDBEditor.server.customRPC(thePaletteDBEditor.parent.nodeId,{paletteMethod:"deleteSwatch",swatchId:$(this).data().swatchId});$(this).remove();});
        
        event.data.updateFromServer();
    },
    
    swaEditClick:function(event){
        thePaletteDBEditor = event.data;
        var selectedswa = $('.ui-selected',event.data.swatchUl);
        var swaitem = selectedswa.first().data();
        
        var colour = {r:swaitem.rVal,g:swaitem.gVal,b:swaitem.bVal};
        new LSDDialogueNodeColour("Edit Swatch Colour",colour,swaitem.swatchId,event.data.swaEditFollow);
    },
    swaEditFollow:function(data,colour){
        thePaletteDBEditor.server.customRPC(thePaletteDBEditor.parent.nodeId,
                                            {paletteMethod:"updateSwatch",
                                            swatchId:data,colour:colour},
                                            thePaletteDBEditor.updateFromServer);
    },
    
    updateFromServer:function(){
        thePaletteDBEditor.parent.updateFromServer();
    }
};


function NumOutEntry(nodeId,server,numOuts){
    this.server = server;
    this.nodeId = nodeId;
    
    this.dialog = $(document.createElement('div'));
    this.dialog.dialog({title:"Set Num Outs", width:200, height:100, modal:true, resizable:false});
    this.dialog.bind('dialogclose',this,function(event){
                     var val = parseInt(event.data.numberBox.val());
                     if(!isNaN(val)){
                     event.data.server.customRPC(event.data.nodeId,{paletteMethod:"setNumOut",numOut:val});
                     thePaletteSamplerEditor.updateFromServer();
                     }
                     });
    
    this.numberBox = $(document.createElement('input'));
    this.numberBox.addClass('CORE_numBox');
    this.numberBox.val(numOuts);
    
    this.dialog.append('<span class="CORE_numLabel"># Outs:</span>');
    this.dialog.append(this.numberBox);
    
}


var thePaletteSamplerEditor;

function PaletteSamplerEditor(nodeId,server){
    thePaletteSamplerEditor = this;
    this.nodeId = nodeId;
    this.server = server;
    
    // Last selected palette
    this.lastSelPal = null;
    
    this.numOuts = null;
    
    this.dialog = $(document.createElement('div'));
    this.dialog.dialog({title:"Palette Sampler",width:500,height:610,modal:true,resizable:false});
    this.dialog.bind('dialogclose',this.dialog,function(event){event.data.remove();lsdApp.reloadCurView();});
    
    var numOutsButton = $(document.createElement('button'));
    numOutsButton.button({label:'# outs'});
    numOutsButton.css('position','absolute');
    numOutsButton.css('right','1em');
    numOutsButton.css('top','1em');
    numOutsButton.css('padding','0');
    numOutsButton.css('font-size','0.7em');
    numOutsButton.click(this,this.setNumOuts);
    this.dialog.append(numOutsButton);
    
    var manualSelCheckDiv = $(document.createElement('div')).addClass('PaletteEditor');
    this.dialog.append(manualSelCheckDiv);
    
    this.manualSelCheck = $(document.createElement('input'));
    this.manualSelCheck.attr('type','checkbox');
    this.manualSelCheck.css('float','left').css('margin-top','0.9em').css('margin-right','0.5em');
    this.manualSelCheck.change(this,this.setSelMode);
    manualSelCheckDiv.append(this.manualSelCheck);
    manualSelCheckDiv.append('<h3>Manual Palette Selection</h3>');
    
    this.editorcontent = $(document.createElement('div'));
    this.dialog.append(this.editorcontent);
    
    var manualSampCheckDiv = $(document.createElement('div')).addClass('PaletteEditor');
    manualSampCheckDiv.css('clear','both');
    this.dialog.append(manualSampCheckDiv);
    
    this.manualSampCheck = $(document.createElement('input'));
    this.manualSampCheck.attr('type','checkbox');
    this.manualSampCheck.css('float','left').css('margin-top','0.9em').css('margin-right','0.5em');
    this.manualSampCheck.css('position','relative').css('z-index','10');
    this.manualSampCheck.change(this,this.setSampMode);
    manualSampCheckDiv.append(this.manualSampCheck);
    manualSampCheckDiv.append('<h3>Manual Sample Mode</h3>');
    
    
    this.samplerStrip = $(document.createElement('div')).css('position','relative').css('top','0.8em');
    this.dialog.append(this.samplerStrip);
    

    var paramSampModeDiv = $(document.createElement('div')).addClass('PaletteEditor');
    paramSampModeDiv.css('clear','left').css('position','relative').css('top','3em');
    this.paramSampMode = $(document.createElement('select'));
    this.paramSampMode.css('margin-top','0.7em').css('margin-left','0.8em');
    this.paramSampMode.append('<option value="clamp">Clamp</option>');
    this.paramSampMode.append('<option value="loop">Loop</option>');
    this.paramSampMode.append('<option value="black">Blackout</option>');
    paramSampModeDiv.append('<h3>Parameter Extrapolation Mode:</h3>');
    paramSampModeDiv.append(this.paramSampMode);
    this.dialog.append(paramSampModeDiv);
    
    this.paramSampMode.change(this,this.setRepeatMode);

    
    this.updateFromServer();
}

PaletteSamplerEditor.prototype = {
    updateFromServer:function(){
        thePaletteSamplerEditor = this;
        this.server.customRPC(this.nodeId,{paletteMethod:"getSampler"},this.handleServerResp);
    },
    
    updateFromServerWrap:function(){
        thePaletteSamplerEditor.server.customRPC(thePaletteSamplerEditor.nodeId,{paletteMethod:"getSampler"},
                                       thePaletteSamplerEditor.handleServerResp);
    },
    
    handleServerResp:function(data){
        var pse = thePaletteSamplerEditor;
        
        if(data.settings.selMode == 0){
            pse.manualSelCheck.attr('checked',true);
        }
        else if(data.settings.selMode == 1){
            pse.manualSelCheck.attr('checked',false);
        }
        
        if(data.settings.sampleMode == 0){
            pse.manualSampCheck.attr('checked',true);
            pse.paramSampMode.attr('disabled','disabled');
        }
        else if(data.settings.sampleMode == 1){
            pse.manualSampCheck.attr('checked',false);
            pse.paramSampMode.attr('disabled',false);
        }
        
        pse.numOuts = data.settings.numOuts;
        
        pse.editorcontent.empty();
        pse.editorcontent.append(new PaletteDBEditor(data.palettes,pse.server,pse));
        
        pse.samplerStrip.empty();
        pse.samplerStrip.append(GradientSamplerStrip(data.curPalette,data.sampleStops,pse.server,pse.nodeId));
        
        if(data.settings.paramSampleRepeatMode == 0){
            pse.paramSampMode.val('clamp');
        }
        else if(data.settings.paramSampleRepeatMode == 1){
            pse.paramSampMode.val('loop');
        }
        else if(data.settings.paramSampleRepeatMode == 2){
            pse.paramSampMode.val('black');
        }
    },
    
    setNumOuts:function(event){
        thePaletteSamplerEditor = event.data;
        new NumOutEntry(event.data.nodeId,event.data.server,event.data.numOuts);
    },
    
    setSelMode:function(event){
        thePaletteSamplerEditor = event.data;
        
        if(event.data.manualSelCheck.attr('checked')){
            event.data.server.customRPC(event.data.nodeId,{paletteMethod:"setSelMode",mode:0},
                                        event.data.updateFromServerWrap);
        }
        else{
            event.data.server.customRPC(event.data.nodeId,{paletteMethod:"setSelMode",mode:1},
                                        event.data.updateFromServerWrap);
        }
    },
    
    setSampMode:function(event){
        thePaletteSamplerEditor = event.data;
        
        if(event.data.manualSampCheck.attr('checked')){
            event.data.server.customRPC(event.data.nodeId,{paletteMethod:"setSampleMode",mode:0},
                                        event.data.updateFromServerWrap);
        }
        else{
            event.data.server.customRPC(event.data.nodeId,{paletteMethod:"setSampleMode",mode:1},
                                        event.data.updateFromServerWrap);
        }
    },
    
    setRepeatMode:function(event){
        var mode = event.data.paramSampMode.val();
        if(mode == 'clamp'){
            event.data.server.customRPC(event.data.nodeId,{paletteMethod:"setRepeatMode",mode:0});
        }
        else if(mode == 'loop'){
            event.data.server.customRPC(event.data.nodeId,{paletteMethod:"setRepeatMode",mode:1});
        }
        else if(mode == 'black'){
            event.data.server.customRPC(event.data.nodeId,{paletteMethod:"setRepeatMode",mode:2});
        }
    }
};


function PalettePlugin_Conf(classIdx,nodeId,server){
    new PaletteSamplerEditor(nodeId,server);
}

PalettePlugin_CoreHead = {confFunc:PalettePlugin_Conf};

