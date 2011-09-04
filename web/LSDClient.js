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


// File-scope variable for the application object
var lsdApp;

// Trig Utils (with 0 angle at top and corrected for upside down y coords)
function cartToPolar(cart){
    var length = Math.sqrt((Math.pow(cart.x,2))+(Math.pow(cart.y,2)));
    var angle = Math.atan(cart.y/cart.x);
    
    if(cart.x<0)
    angle = Math.PI*1.5-angle;
    else
    angle = Math.PI*0.5-angle;
    
    return {length:length,angle:angle};
}
function polarToCart(polar){
    var x = polar.length*Math.sin(polar.angle);
    var y = -polar.length*Math.cos(polar.angle);
    
    return {x:x,y:y};
}

// Prototype for client application
function LSDApp(){
    this.canvas = $('#canv');
    
    this.UIstack = new Array();
    this.UItop = null;
    
    this.server = null;
    
    this.psArr = new Array();
    
    this.bodyArea = $(document.createElement('div'));
    this.bodyArea.addClass('lsdBodyArea');
    this.canvas.append(this.bodyArea);
    
    this.pageBar = null;
    
    this.activePatchSpace = null;
    
    this.facadeEditDialogue = null;
    
    this.nodeColourDialogue = null;
    
    this.patchPartitionDialogue = null;
    
    // When a wire connection is made, this
    // provides a quick, short-term method of applying the wireId
    this.lastAddedWire = null;
}

LSDApp.prototype = {
    go:function(){
        this.pageBar = new LSDPageBar(this);
        
        this.server = new LSDServer('rpc');
                
        this.canvas.append(this.pageBar.getJquery());
        
        this.library = new LSDLibrary(this);
        this.library.updateFromServer();
        this.library.hideLibrary();
        this.canvas.append(this.library.getJquery());
        
        //this.homepage = new LSDHome(this);
        //this.pageBar.uiPush(this.homepage.getJquery(),'Home',this.homepage.updateFromServer,this.homepage);
        //this.homepage.updateFromServer();
        
        this.reqHome();
        
        //this.addLoadingIndicator();
    },
    reqHome:function(){
        lsdApp.library.hideLibrary();
        //lsdApp.bodyArea.empty();
        lsdApp.addLoadingIndicator();
        lsdApp.server.getPartitions(lsdApp.recvHome);
    },
    reqPS:function(event){
        var psId = event.data.psId;
        //lsdApp.bodyArea.empty();
        lsdApp.addLoadingIndicator();
        lsdApp.server.getPatchSpace(psId,lsdApp.recvPS);
    },
    recvHome:function(data){
        lsdApp.library.hideLibrary();
        lsdApp.pageBar.libButton.button('disable');
        lsdApp.bodyArea.empty();
        var home = new LSDHome(lsdApp,data);
        lsdApp.pageBar.uiPush(home.jq,"Home",lsdApp.reqHome);
    },
    recvPS:function(data){
        lsdApp.bodyArea.empty();
        var ps = new LSDPatchSpace(data);
        lsdApp.activePatchSpace = ps;
        lsdApp.pageBar.libButton.button('enable');
        lsdApp.pageBar.uiPush(ps.divE,data.name,lsdApp.reqPS,{data:{psId:data.psId}});
    },
    reloadCurView:function(){
        var curIdx = lsdApp.pageBar.uiArr.length-1;
        lsdApp.pageBar.uiSlice(curIdx);
    },
    addLoadingIndicator:function(){
        var activityContainer = $(document.createElement('div')).addClass('activityContainer').append('<span>Loading...</span>');
        activityContainer.append($(document.createElement('div')).addClass('activityIndicator'));
        this.bodyArea.append(activityContainer);
    },
    quickWire:function(data){
        if(data.error)
            lsdApp.lastAddedWire.removeWire(lsdApp.activePatchSpace);
        else
            lsdApp.lastAddedWire.wireId = data.wireId;
    },
    handleZoomTimeout:function(){
        var ps = lsdApp.activePatchSpace;
        lsdApp.server.panPatchSpace(ps.psId,ps.x,ps.y,ps.scale);
    },
};

// ************************

// Prototype for PageBar
function LSDPageBar(app){
    this.app = app;
    this.uiArr = new Array();
    
    this.jq = $(document.createElement('div'));
    this.jq.addClass('lsdPageBar');
    //this.jq.append('<p>hello!</p>');
    
    this.itemList = $(document.createElement('div'));
    this.jq.append(this.itemList);
    
    this.patchButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-gear'},text:false});
    this.patchButton.css('position','absolute').css('right','50px').css('top','12px').css('width','25px').css('height','25px');
    this.patchButton.click(this,function(event){LSDDialoguePreferences(event.data.app.server);});
    
    this.libButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-note'},text:false});
    this.libButton.css('position','absolute').css('right','80px').css('top','12px').css('width','25px').css('height','25px');
    this.libButton.click(this,function(event){event.data.app.library.toggleLibrary();});
    
    this.jq.append(this.patchButton);
    this.jq.append(this.libButton);
    
    //this.testPopulate();
}

LSDPageBar.prototype = {
    getJquery:function(){
        return this.jq;
    },
    makeSep:function(){
        return $(document.createElement('div')).addClass('lsdPageBarSep');
    },
    uiPush:function(jq,name,activateCB,activateData){
        this.app.bodyArea.empty().append(jq);
        
        if(this.uiArr.length>0)
            this.itemList.append(this.makeSep());
        
        var barItem = $(document.createElement('span')).append(name);
        barItem.click({pb:this,idx:this.uiArr.length},this.handleItemClick);
        this.itemList.append(barItem);
        
        this.uiArr.push({barItem:barItem,activateCB:activateCB,activateData:activateData});
    },
    uiSlice:function(idx){
        var nIdx = parseInt(idx);
        
        //if(this.uiArr.length<=(nIdx+1))
        //	return;
        
        //this.app.bodyArea.empty();
        
        var activateCB = this.uiArr[idx].activateCB;
        var activateData = this.uiArr[idx].activateData;
        
        activateCB(activateData);
                
        var newArr = new Array();
        for(var i=0;i<idx;++i){
            newArr.push(this.uiArr[i]);
        }
        
        this.uiArr = newArr;
        
        this.itemList.empty();
        for(var i in this.uiArr){
            if(i>0)
                this.itemList.append(this.makeSep());
            this.itemList.append(this.uiArr[i].barItem);
            this.uiArr[i].barItem.click({pb:this,idx:i},this.handleItemClick);
        }
    },
    handleItemClick:function(event){
        event.data.pb.uiSlice(event.data.idx);
    },
    
    testPopulate:function(){
        for(var i=0;i<5;++i){
            var item = $(document.createElement('div')).append(i).css('font-size','30em').css('color','white');
            this.uiPush(item,'Item: '+i);
        }
    }
};

// ************************

// Prototype for LSD's home page

function LSDHome(app,data){
    this.app = app;
    
    this.jq = $(document.createElement('div'));
    this.jq.addClass('lsdHome');
    
    if(data)
        this.handleUpdateResp(this,data);
}

LSDHome.prototype = {
    getJquery:function(){
        return this.jq;
    },
    
    handleUpdateResp:function(home,data){
        home.jq.empty();
        
        for(var i in data.partitions){
            var partData = data.partitions[i];
            
            var partBox = $(document.createElement('div'));
            partBox.addClass('lsdPartBox');
            if(partData.imageUrl)
                partBox.css('background-image','url(\'../uploads/'+partData.imageUrl+'\')');
            
            partBox.click({app:lsdApp,psId:partData.psId,name:partData.partName},lsdApp.reqPS);
            
            home.jq.append(partBox);
        }
    }
};

// ************************

// Prototype for Class Library
function LSDLibrary(app){
    this.app = app;
    
    this.jq = $(document.createElement('div'));
    this.jq.addClass('lsdLibraryClip');
    this.jq.css('pointer-events','none');

    this.libCont = $(document.createElement('div'));
    this.libCont.addClass('lsdLibrary');
    this.jq.append(this.libCont);
    
    this.liblist = $(document.createElement('ul'));
    this.libCont.append(this.liblist);
    
    this.liblist.append($(document.createElement('li')).addClass('dummy'));
    
    for(var i=0;i<20;++i){
        this.liblist.append(this.makeFacadeItem());
    }
}

LSDLibrary.prototype = {
    getJquery:function(){
        return this.jq;
    },
    updateFromServer:function(data){
        var home;
        if(data)
            home = data;
        else
            home = this;
        
        home.app.server.getLibrary(home.handleLibraryResp);
    },
    handleLibraryResp:function(data){
        var library = lsdApp.library;
        
        library.liblist.empty().append($(document.createElement('div')).addClass('dummy'));
        library.liblist.append(library.makeFacadeItem());
        
        for(var i in data.classes){
            var itemData = data.classes[i];
            
            var libItem = $(document.createElement('li')).addClass('ui-widget-content');
            libItem.append(itemData.className).data(itemData);
            libItem.draggable({helper:'clone',revert:'invalid',appendTo:lsdApp.canvas});
 
            library.liblist.append(libItem);
        }

	library.liblist.append($(document.createElement('div')).addClass('dummy'));
    },
    makeFacadeItem:function(){
        var facadeItem = $(document.createElement('li'));
        facadeItem.addClass('ui-widget-content').append('Facade').data({facade:true});
        facadeItem.draggable({helper:'clone',revert:'invalid',appendTo:this.app.canvas});
        
        return facadeItem;
    },
    showLibrary:function(){
        this.libCont.css('-webkit-transform','translate(0px,0px)');
        this.libCont.css('-moz-transform','translate(0px,0px)');
        this.shown = true;
        this.jq.css('pointer-events','auto');
    },
    hideLibrary:function(){
        this.libCont.css('-webkit-transform','translate(300px,0px)');
        this.libCont.css('-moz-transform','translate(300px,0px)');
        this.shown = false;
        this.jq.css('pointer-events','none');
    },
    toggleLibrary:function(){
        if(this.shown)
            this.hideLibrary();
        else
            this.showLibrary();
    }
};


// ************************

// Prototype for Node
function LSDNode(parps,nodeId,classObj,name,colour){
    this.parps = parps;
    this.nodeId = nodeId;
    this.classObj = classObj;
    this.name = name;
    this.colour = colour;
    
    this.inArr = new Array();
    this.outArr = new Array();
    
    this.x = 0;
    this.y = 0;

    
    this.nodeSvg = document.createElementNS('http://www.w3.org/2000/svg','g');
    this.setPosition(50,100);
    

    
    // Add Drop Shadow
    this.ds = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.ds.setAttribute('fill','url(#dsGrad)');
    this.ds.setAttribute('x','-80');
    this.ds.setAttribute('y','-60');
    this.ds.setAttribute('width','460');
    this.ds.setAttribute('height','320');
    this.ds.setAttribute('pointer-events','none');
    this.nodeSvg.appendChild(this.ds);

    
    // add clip rect path
    var clipPath = document.createElementNS('http://www.w3.org/2000/svg','clipPath');
    clipPath.setAttribute('id','clipRect_'+nodeId);
    this.nodeSvg.appendChild(clipPath);
    
    // add rect for above clip path
    this.clipRect = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.clipRect.setAttribute('style','position:absolute;fill:#f55');
    this.clipRect.setAttribute('width','300');
    this.clipRect.setAttribute('height','200');
    this.clipRect.setAttribute('rx','8');
    this.clipRect.setAttribute('ry','8');
    clipPath.appendChild(this.clipRect);

    
    
    // Rect content group
    var rectGroup = document.createElementNS('http://www.w3.org/2000/svg','g');
    this.nodeSvg.appendChild(rectGroup);
    
    
    
    // Add fill rect colour and clip it
    this.fillRectCol = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.fillRectCol.setAttribute('clip-path','url(#clipRect_'+nodeId+')');
    this.fillRectCol.setAttribute('style','fill:rgba('+Math.floor(colour.r*255)+','+
                                  Math.floor(colour.g*255)+','+Math.floor(colour.b*255)+',1)');
    this.fillRectCol.setAttribute('width','300');
    this.fillRectCol.setAttribute('height','200');
    rectGroup.appendChild(this.fillRectCol);
    
    // Add fill rect shading and clip it
    this.fillRectShad = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.fillRectShad.setAttribute('clip-path','url(#clipRect_'+nodeId+')');
    this.fillRectShad.setAttribute('style','fill:url(#nodeBgGrad)');
    this.fillRectShad.setAttribute('width','300');
    this.fillRectShad.setAttribute('height','200');
    rectGroup.appendChild(this.fillRectShad);
    

    
    // Add bevel shading
    this.bevelShader = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.bevelShader.setAttribute('clip-path','url(#clipRect_'+nodeId+')');
    this.bevelShader.setAttribute('style','fill:url(#outBevelGrad)');
    this.bevelShader.setAttribute('width','300');
    this.bevelShader.setAttribute('height','200');
    rectGroup.appendChild(this.bevelShader);
    
    // Add node's wave effect
    this.wave = document.createElementNS('http://www.w3.org/2000/svg','path');
    this.wave.setAttribute('id','nodeWavePath');
    this.wave.setAttribute('d','m -26.75085,150 c 0,0 81.123575,28.30275 186.75085,0 105.62724,-28.302694 187.64436,8.93544 187.64436,8.93544 l 0,-300 -376.182309,0 z');
    this.wave.setAttribute('style','fill:url(#nodeWaveGrad);stroke:none;');
    this.wave.setAttribute('clip-path','url(#clipRect_'+nodeId+')');
    rectGroup.appendChild(this.wave);
    
    // Add standard node icons
    this.trash = document.createElementNS('http://www.w3.org/2000/svg','image');
    this.trash.setAttribute('x','260');
    this.trash.setAttribute('width','35');
    this.trash.setAttribute('height','35');
    this.trash.setAttributeNS('http://www.w3.org/1999/xlink','xlink:href','../images/icon_trash.svg');
    this.nodeSvg.appendChild(this.trash);
    $(this.trash).click(this,this.handleTrash);
    
    this.gear = document.createElementNS('http://www.w3.org/2000/svg','image');
    this.gear.setAttribute('x','230');
    this.gear.setAttribute('width','32');
    this.gear.setAttribute('height','32');
    this.gear.setAttributeNS('http://www.w3.org/1999/xlink','xlink:href','../images/icon_gear.svg');
    this.nodeSvg.appendChild(this.gear);
    $(this.gear).click(this,this.handleDoubleClick);
    
    this.palette = document.createElementNS('http://www.w3.org/2000/svg','image');
    this.palette.setAttribute('x','195');
    this.palette.setAttribute('width','35');
    this.palette.setAttribute('height','35');
    this.palette.setAttributeNS('http://www.w3.org/1999/xlink','xlink:href','../images/icon_palette.svg');
    this.nodeSvg.appendChild(this.palette);
    $(this.palette).click(this,this.handlePalette);
    
    // Add name text
    this.nameText = document.createElementNS('http://www.w3.org/2000/svg','text');
    this.nameText.textContent = this.name;
    this.nameText.setAttribute('class','lsdNodeCommentText');
    this.nameText.setAttribute('x','10');
    this.nodeSvg.appendChild(this.nameText);
    $(this.nameText).dblclick(this,this.handleRename);
    
    
    this.nodeSvgJq = $(rectGroup);
    this.nodeSvgJq.mousedown(this,this.handleMouseDown);
    this.nodeSvgJq.dblclick(this,this.handleDoubleClick);
    
    
}

LSDNode.prototype = {
    getSvg:function(){
        return this.nodeSvg;
    },
    
    setPosition:function(x,y){
        this.x = x;
        this.y = y;
        
        this.nodeSvg.setAttribute('transform','translate('+x+','+y+')');
        
        for(var i in this.outArr){
            this.outArr[i].updateWirePos();
        }
        
        for(var i in this.inArr){
            this.inArr[i].updateWirePos();
        }
    },
    
    scaleHeight:function(height){
        height = Math.max(0,height);
        height += 60;
        
        this.ds.setAttribute('height',height+120);
        
        this.clipRect.setAttribute('height',height);
        
        var shadAdj = (Math.min(200,height)/2)-100;
        
        this.fillRectCol.setAttribute('height',height);
        this.fillRectShad.setAttribute('height',height-shadAdj);
        this.fillRectShad.setAttribute('y',shadAdj);
        
        this.bevelShader.setAttribute('height',height);
        
        this.wave.setAttribute('d','m -26.75085,'+(height-50)+' c 0,0 81.123575,28.30275 186.75085,0 105.62724,-28.302694 187.64436,8.93544 187.64436,8.93544 l 0,'+(-100-height)+' -376.182309,0 z');
        
        this.trash.setAttribute('y',height-35);
        this.gear.setAttribute('y',height-34);
        this.palette.setAttribute('y',height-35);
        
        this.nameText.setAttribute('y',height-10);
    },
    
    addInput:function(inIn){
        var inSvg = inIn.getSvg();
        this.inArr.push(inIn);
        this.nodeSvg.appendChild(inSvg);
        this.parps.inArr[inIn.inId] = inIn;
    },
    
    flowInputs:function(){
        var curY = 10;
        
        for(i in this.inArr){
            var tempIn = this.inArr[i];
            tempIn.setPosition(10,curY);
            //tempIn.setAttribute('transform','translate(10,'+curY+')');
            curY += 17;
        }
        
        this.maxPlugstripHeight = curY;
    },
    
    addOutput:function(outIn){
        var outSvg = outIn.getSvg();
        this.outArr.push(outIn);
        this.nodeSvg.appendChild(outSvg);
        this.parps.outArr[outIn.outId] = outIn;
    },
    
    flowOutputs:function(){
        var curY = 10;
        
        for(i in this.outArr){
            var tempOut = this.outArr[i];
            tempOut.setPosition(290,curY);
            //tempOut.setAttribute('transform','translate(290,'+curY+')');
            curY += 17;
        }
        
        if(curY > this.maxPlugstripHeight)
            this.scaleHeight(curY);
        else
            this.scaleHeight(this.maxPlugstripHeight);
    },
    
    handleDoubleClick:function(event){
        //alert(event.data.classObj.pluginId);
        LSD_PLUGIN_TABLE[event.data.classObj.pluginId].confFunc(event.data.classObj.classIdx,
                                                                event.data.nodeId,lsdApp.server);
    },
    
    handleMouseDown:function(event){
        event.preventDefault();
        event.stopPropagation();
        
        event.data.nodeSvgJq.unbind('mousedown',event.data.handleMouseDown);
        
        var doc = $(document);
        doc.mousemove(event.data,event.data.handleMouseMove);
        doc.mouseup(event.data,event.data.handleMouseUp);
        
        event.data.holdOffsetX = (event.pageX/event.data.parps.scale)-event.data.x;
        event.data.holdOffsetY = (event.pageY/event.data.parps.scale)-event.data.y;
        
        event.data.getFocus(event.data.nodeSvg);
        event.data.setPosition(event.data.x,event.data.y);
    },
    
    handleMouseMove:function(event){
        event.preventDefault();
        
        var x = (event.pageX/event.data.parps.scale)-event.data.holdOffsetX;
        var y = (event.pageY/event.data.parps.scale)-event.data.holdOffsetY;
        
        event.data.setPosition(x,y);
    },
    
    handleMouseUp:function(event){
        var doc = $(document);
        doc.unbind('mousemove',event.data.handleMouseMove);
        doc.unbind('mouseup',event.data.handleMouseUp);
        
        event.data.nodeSvgJq.mousedown(event.data,event.data.handleMouseDown);
        lsdApp.server.positionNode(event.data.nodeId,event.data.x,event.data.y);
    },
    
    getFocus:function(){
        this.parps.giveNodeFocus(this.nodeSvg);
    },
    
    handleTrash:function(event){
        lsdApp.server.deleteNode(event.data.nodeId,function(){lsdApp.reloadCurView();});
    },
    
    handlePalette:function(event){
        new LSDDialogueNodeColour("Colour Node",event.data.colour,event.data.nodeId,event.data.colourFollow);
    },
    
    colourFollow:function(data,colour){
        lsdApp.server.setNodeColour(data,colour,function(){lsdApp.reloadCurView();});
    },
        
    handleRename:function(event){
        new LSDDialogueName("Rename Node","Node Name:",event.data,event.data.renameFollowup,event.data.name);
    },
        
    renameFollowup:function(data,name){
        lsdApp.server.updateNodeName(data.nodeId,name,function(){lsdApp.reloadCurView();});
    }
};

// ************************

// Prototype for FacadeNode
function LSDFacadeNode(parps,nodeId,name,colour){
    this.parps = parps;
    this.nodeId = nodeId;
    this.name = name;
    this.colour = colour;
    
    this.inArr = new Array();
    this.outArr = new Array();
    
    this.x = 0;
    this.y = 0;
    
    
    this.nodeSvg = document.createElementNS('http://www.w3.org/2000/svg','g');
    this.setPosition(50,100);
    
    
    
    // Add Drop Shadow
    this.ds = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.ds.setAttribute('fill','url(#dsGrad)');
    this.ds.setAttribute('x','-80');
    this.ds.setAttribute('y','-80');
    this.ds.setAttribute('width','460');
    this.ds.setAttribute('height','320');
    this.ds.setAttribute('pointer-events','none');
    this.nodeSvg.appendChild(this.ds);
    
    
    // add clip rect path
    var clipPath = document.createElementNS('http://www.w3.org/2000/svg','clipPath');
    clipPath.setAttribute('id','clipFacRect_'+nodeId);
    this.nodeSvg.appendChild(clipPath);
    
    // add rect for above clip path
    this.clipRect = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.clipRect.setAttribute('style','position:absolute;fill:#f55');
    this.clipRect.setAttribute('width','300');
    this.clipRect.setAttribute('height','200');
    this.clipRect.setAttribute('rx','0');
    this.clipRect.setAttribute('ry','0');
    clipPath.appendChild(this.clipRect);
    
    
    
    // Rect content group
    var rectGroup = document.createElementNS('http://www.w3.org/2000/svg','g');
    this.nodeSvg.appendChild(rectGroup);
    
    
    
    // Add fill rect colour and clip it
    this.fillRectCol = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.fillRectCol.setAttribute('clip-path','url(#clipFacRect_'+nodeId+')');
    this.fillRectCol.setAttribute('style','fill:rgba('+Math.floor(colour.r*255)+','+
                                  Math.floor(colour.g*255)+','+Math.floor(colour.b*255)+',1)');
    this.fillRectCol.setAttribute('width','300');
    this.fillRectCol.setAttribute('height','200');
    rectGroup.appendChild(this.fillRectCol);
    
    // Add fill rect shading and clip it
    this.fillRectShad = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.fillRectShad.setAttribute('clip-path','url(#clipFacRect_'+nodeId+')');
    this.fillRectShad.setAttribute('style','fill:url(#nodeBgGrad)');
    this.fillRectShad.setAttribute('width','300');
    this.fillRectShad.setAttribute('height','200');
    rectGroup.appendChild(this.fillRectShad);
    
    
    
    // Add bevel shading
    this.bevelShader = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.bevelShader.setAttribute('clip-path','url(#clipFacRect_'+nodeId+')');
    this.bevelShader.setAttribute('style','fill:url(#outBevelGrad)');
    this.bevelShader.setAttribute('width','300');
    this.bevelShader.setAttribute('height','200');
    rectGroup.appendChild(this.bevelShader);
    
    // Add node's wave effect
    this.wave = document.createElementNS('http://www.w3.org/2000/svg','path');
    this.wave.setAttribute('id','nodeWavePath');
    this.wave.setAttribute('d','m -26.75085,150 c 0,0 81.123575,28.30275 186.75085,0 105.62724,-28.302694 187.64436,8.93544 187.64436,8.93544 l 0,-300 -376.182309,0 z');
    this.wave.setAttribute('style','fill:url(#nodeWaveGrad);stroke:none;');
    this.wave.setAttribute('clip-path','url(#clipFacRect_'+nodeId+')');
    rectGroup.appendChild(this.wave);
    
    // Add standard node icons
    this.trash = document.createElementNS('http://www.w3.org/2000/svg','image');
    this.trash.setAttribute('x','260');
    this.trash.setAttribute('width','35');
    this.trash.setAttribute('height','35');
    this.trash.setAttributeNS('http://www.w3.org/1999/xlink','xlink:href','../images/icon_trash.svg');
    this.nodeSvg.appendChild(this.trash);
    $(this.trash).click(this,this.handleTrash);
    
    this.gear = document.createElementNS('http://www.w3.org/2000/svg','image');
    this.gear.setAttribute('x','230');
    this.gear.setAttribute('width','32');
    this.gear.setAttribute('height','32');
    this.gear.setAttributeNS('http://www.w3.org/1999/xlink','xlink:href','../images/icon_gear.svg');
    this.nodeSvg.appendChild(this.gear);
    $(this.gear).click(this,this.handleGear);
    
    this.palette = document.createElementNS('http://www.w3.org/2000/svg','image');
    this.palette.setAttribute('x','195');
    this.palette.setAttribute('width','35');
    this.palette.setAttribute('height','35');
    this.palette.setAttributeNS('http://www.w3.org/1999/xlink','xlink:href','../images/icon_palette.svg');
    this.nodeSvg.appendChild(this.palette);
    $(this.palette).click(this,this.handlePalette);
    
    // Add name text
    this.nameText = document.createElementNS('http://www.w3.org/2000/svg','text');
    this.nameText.textContent = this.name;
    this.nameText.setAttribute('class','lsdNodeCommentText');
    this.nameText.setAttribute('x','10');
    this.nodeSvg.appendChild(this.nameText);
    $(this.nameText).dblclick(this,this.handleRename);
    
    
    this.nodeSvgJq = $(rectGroup);
    this.nodeSvgJq.mousedown(this,this.handleMouseDown);
    this.nodeSvgJq.dblclick({app:lsdApp,psId:this.nodeId,name:"Test Facade"},lsdApp.reqPS);
    
    
}

LSDFacadeNode.prototype = {
    getSvg:function(){
        return this.nodeSvg;
    },
        
    setPosition:function(x,y){
        this.x = x;
        this.y = y;
        
        this.nodeSvg.setAttribute('transform','translate('+x+','+y+')');
        
        for(var i in this.outArr){
            this.outArr[i].updateWirePos();
        }
        
        for(var i in this.inArr){
            this.inArr[i].updateWirePos();
        }
    },
        
    scaleHeight:function(height){
        height = Math.max(0,height);
        height += 60;
        
        this.ds.setAttribute('height',height+80);
        
        this.clipRect.setAttribute('height',height);
        
        var shadAdj = (Math.min(200,height)/2)-100;
        
        this.fillRectCol.setAttribute('height',height);
        this.fillRectShad.setAttribute('height',height-shadAdj);
        this.fillRectShad.setAttribute('y',shadAdj);
        
        this.bevelShader.setAttribute('height',height);
        
        this.wave.setAttribute('d','m -26.75085,'+(height-50)+' c 0,0 81.123575,28.30275 186.75085,0 105.62724,-28.302694 187.64436,8.93544 187.64436,8.93544 l 0,'+(-100-height)+' -376.182309,0 z');
        
        this.trash.setAttribute('y',height-35);
        this.gear.setAttribute('y',height-34);
        this.palette.setAttribute('y',height-35);
        
        this.nameText.setAttribute('y',height-10);
    },
        
    addInput:function(inIn){
        var inSvg = inIn.getSvg();
        this.inArr.push(inIn);
        this.nodeSvg.appendChild(inSvg);
        this.parps.inArr[inIn.inId] = inIn;
    },
        
    flowInputs:function(){
        var curY = 10;
        
        for(i in this.inArr){
            var tempIn = this.inArr[i];
            tempIn.setPosition(10,curY);
            //tempIn.setAttribute('transform','translate(10,'+curY+')');
            curY += 17;
        }
        
        this.maxPlugstripHeight = curY;
    },
        
    addOutput:function(outIn){
        var outSvg = outIn.getSvg();
        this.outArr.push(outIn);
        this.nodeSvg.appendChild(outSvg);
        this.parps.outArr[outIn.outId] = outIn;
    },
        
    flowOutputs:function(){
        var curY = 10;
        
        for(i in this.outArr){
            var tempOut = this.outArr[i];
            tempOut.setPosition(290,curY);
            //tempOut.setAttribute('transform','translate(290,'+curY+')');
            curY += 17;
        }
        
        if(curY > this.maxPlugstripHeight)
            this.scaleHeight(curY);
        else
            this.scaleHeight(this.maxPlugstripHeight);
    },
        
    handleMouseDown:function(event){
        event.preventDefault();
        event.stopPropagation();
        
        event.data.nodeSvgJq.unbind('mousedown',event.data.handleMouseDown);
        
        var doc = $(document);
        doc.mousemove(event.data,event.data.handleMouseMove);
        doc.mouseup(event.data,event.data.handleMouseUp);
        
        event.data.holdOffsetX = (event.pageX/event.data.parps.scale)-event.data.x;
        event.data.holdOffsetY = (event.pageY/event.data.parps.scale)-event.data.y;
        
        event.data.getFocus(event.data.nodeSvg);
        event.data.setPosition(event.data.x,event.data.y);
    },
        
    handleMouseMove:function(event){
        event.preventDefault();
        
        var x = (event.pageX/event.data.parps.scale)-event.data.holdOffsetX;
        var y = (event.pageY/event.data.parps.scale)-event.data.holdOffsetY;
        
        event.data.setPosition(x,y);
    },
        
    handleMouseUp:function(event){
        var doc = $(document);
        doc.unbind('mousemove',event.data.handleMouseMove);
        doc.unbind('mouseup',event.data.handleMouseUp);
        
        event.data.nodeSvgJq.mousedown(event.data,event.data.handleMouseDown);
        lsdApp.server.positionFacade(event.data.nodeId,event.data.x,event.data.y);
    },
        
    getFocus:function(){
        this.parps.giveNodeFocus(this.nodeSvg);
    },
        
    handleTrash:function(event){
        lsdApp.server.deleteFacade(event.data.nodeId,function(){lsdApp.reloadCurView();});
    },
        
    handleGear:function(event){
        lsdApp.facadeEditDialogue = new LSDDialogueFacadeEdit(lsdApp.server,event.data.nodeId);
    },
        
    handlePalette:function(event){
        new LSDDialogueNodeColour("Colour Facade",event.data.colour,event.data.nodeId,event.data.colourFollow);
    },
        
    colourFollow:function(data,colour){
        lsdApp.server.setFacadeColour(data,colour,function(){lsdApp.reloadCurView();});
    },
        
    handleRename:function(event){
        new LSDDialogueName("Rename Facade","Facade Name",event.data,event.data.renameFollowup,event.data.name);
    },
        
    renameFollowup:function(data,name){
        lsdApp.server.updateFacadeName(data.nodeId,name,function(){lsdApp.reloadCurView();});
    }
};

// ************************

// Prototype for LSDFacadeIns
function LSDFacadeIns(assps,insArr){
    this.assps = assps;
    
    this.outArr = new Array();
    
    this.x = 0;
    this.y = 0;
    
    
    this.nodeSvg = document.createElementNS('http://www.w3.org/2000/svg','g');
    this.setPosition(-20,250);
    
    this.nodeSvgJq = $(this.nodeSvg);
    //this.nodeSvgJq.mousedown(this,this.handleMouseDown);
    
    
    // Add Drop Shadow
    this.ds = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.ds.setAttribute('fill','url(#dsGrad)');
    this.ds.setAttribute('x','-40');
    this.ds.setAttribute('y','-40');
    this.ds.setAttribute('width','180');
    this.ds.setAttribute('height','280');
    this.ds.setAttribute('pointer-events','none');
    this.nodeSvg.appendChild(this.ds);
    
    // add clip rect path
    var clipPath = document.createElementNS('http://www.w3.org/2000/svg','clipPath');
    clipPath.setAttribute('id','clipRectFacIns_'+this.assps.psId);
    this.nodeSvg.appendChild(clipPath);
    
    // add rect for above clip path
    this.clipRect = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.clipRect.setAttribute('style','position:absolute;fill:#f55');
    this.clipRect.setAttribute('width','100');
    this.clipRect.setAttribute('height','200');
    this.clipRect.setAttribute('rx','5');
    this.clipRect.setAttribute('ry','5');
    clipPath.appendChild(this.clipRect);

    
    
    // Rect content group
    var rectGroup = document.createElementNS('http://www.w3.org/2000/svg','g');
    this.nodeSvg.appendChild(rectGroup);
    
    
    // Add fill rect colour and clip it
    this.fillRectCol = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.fillRectCol.setAttribute('clip-path','url(#clipRectFacIns_'+this.assps.psId+')');
    this.fillRectCol.setAttribute('style','fill:#00f');
    this.fillRectCol.setAttribute('width','100');
    this.fillRectCol.setAttribute('height','200');
    rectGroup.appendChild(this.fillRectCol);
    
    // Add fill rect shad and clip it
    this.fillRectShad = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.fillRectShad.setAttribute('clip-path','url(#clipRectFacIns_'+this.assps.psId+')');
    this.fillRectShad.setAttribute('style','fill:url(#nodeBgGrad)');
    this.fillRectShad.setAttribute('width','100');
    this.fillRectShad.setAttribute('height','200');
    rectGroup.appendChild(this.fillRectShad);
    
    
    
    // Add bevel shading
    this.bevelShader = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.bevelShader.setAttribute('clip-path','url(#clipRectFacIns_'+this.assps.psId+')');
    this.bevelShader.setAttribute('style','fill:url(#outBevelGrad)');
    this.bevelShader.setAttribute('width','100');
    this.bevelShader.setAttribute('height','200');
    rectGroup.appendChild(this.bevelShader);
    
    // Add node's wave effect
    this.wave = document.createElementNS('http://www.w3.org/2000/svg','path');
    this.wave.setAttribute('id','nodeWavePath');
    this.wave.setAttribute('d','m -26.75085,102.36218 c 0,0 81.123575,28.30275 186.75085,0 105.62724,-28.302694 187.64436,8.93544 187.64436,8.93544 l 0,-113.4801638 -376.182309,0 z');
    this.wave.setAttribute('style','fill:url(#nodeWaveGrad);stroke:none;');
    this.wave.setAttribute('clip-path','url(#clipRectFacIns_'+this.assps.psId+')');
    rectGroup.appendChild(this.wave);
    
    
    // Add icon
    this.inIcon = document.createElementNS('http://www.w3.org/2000/svg','image');
    this.inIcon.setAttribute('x','37');
    this.inIcon.setAttribute('width','25');
    this.inIcon.setAttribute('height','25');
    this.inIcon.setAttributeNS('http://www.w3.org/1999/xlink','xlink:href','../images/icon_in.svg');
    this.nodeSvg.appendChild(this.inIcon);
    
    
    for(var i in insArr){
        var newOut = new LSDOutput(this,0,insArr[i].inId);
        newOut.setText(insArr[i].inName);
        this.addOutput(newOut);
    }
    this.flowOutputs();
    
}

LSDFacadeIns.prototype = {
getSvg:function(){
    return this.nodeSvg;
},
    
scaleHeight:function(height){
    height = Math.max(0,height);
    height += 50;
    
    this.ds.setAttribute('height',height+110);
    
    this.clipRect.setAttribute('height',height);
    
    var shadAdj = (Math.min(200,height)/2)-100;
    
    this.fillRectCol.setAttribute('height',height);
    this.fillRectShad.setAttribute('height',height-shadAdj);
    this.fillRectShad.setAttribute('y',shadAdj);
    
    this.bevelShader.setAttribute('height',height);
    
    this.wave.setAttribute('d','m -26.75085,'+(height-50)+' c 0,0 81.123575,28.30275 186.75085,0 105.62724,-28.302694 187.64436,8.93544 187.64436,8.93544 l 0,'+(-100-height)+' -376.182309,0 z');
    
    this.inIcon.setAttribute('y',height-30);
},
    
setPosition:function(x,y){
    this.x = x;
    this.y = y;
    
    this.nodeSvg.setAttribute('transform','translate('+x+','+y+')');
    
    for(var i in this.outArr){
        this.outArr[i].updateWirePos();
    }
    
},
    
    
addOutput:function(outIn){
    var outSvg = outIn.getSvg();
    this.outArr[outIn.outId] = outIn;
    this.nodeSvg.appendChild(outSvg);
    this.assps.outArr[outIn.outId] = outIn;
},
    
flowOutputs:function(){
    var curY = 10;
    
    for(i in this.outArr){
        var tempOut = this.outArr[i];
        tempOut.setPosition(90,curY);
        //tempOut.setAttribute('transform','translate(290,'+curY+')');
        curY += 17;
    }
    
    this.scaleHeight(curY);
},
    
    
};

// ************************

// Prototype for LSDFacadeOuts
function LSDFacadeOuts(assps,outsArr){
    this.assps = assps;
    
    this.inArr = new Array();
    
    this.x = 0;
    this.y = 0;
    
    
    this.nodeSvg = document.createElementNS('http://www.w3.org/2000/svg','g');
    this.setPosition(950,250);
    
    this.nodeSvgJq = $(this.nodeSvg);
    //this.nodeSvgJq.mousedown(this,this.handleMouseDown);
    
    
    // Add Drop Shadow
    this.ds = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.ds.setAttribute('fill','url(#dsGrad)');
    this.ds.setAttribute('x','-40');
    this.ds.setAttribute('y','-40');
    this.ds.setAttribute('width','180');
    this.ds.setAttribute('height','280');
    this.ds.setAttribute('pointer-events','none');
    this.nodeSvg.appendChild(this.ds);
    
    // add clip rect path
    var clipPath = document.createElementNS('http://www.w3.org/2000/svg','clipPath');
    clipPath.setAttribute('id','clipRectFacOuts_'+this.assps.psId);
    this.nodeSvg.appendChild(clipPath);
    
    // add rect for above clip path
    this.clipRect = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.clipRect.setAttribute('style','position:absolute;fill:#f55');
    this.clipRect.setAttribute('width','100');
    this.clipRect.setAttribute('height','200');
    this.clipRect.setAttribute('rx','5');
    this.clipRect.setAttribute('ry','5');
    clipPath.appendChild(this.clipRect);
    
    
    
    // Rect content group
    var rectGroup = document.createElementNS('http://www.w3.org/2000/svg','g');
    this.nodeSvg.appendChild(rectGroup);
    
    
    // Add fill rect colour and clip it
    this.fillRectCol = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.fillRectCol.setAttribute('clip-path','url(#clipRectFacOuts_'+this.assps.psId+')');
    this.fillRectCol.setAttribute('style','fill:#00f');
    this.fillRectCol.setAttribute('width','100');
    this.fillRectCol.setAttribute('height','200');
    rectGroup.appendChild(this.fillRectCol);
    
    // Add fill rect shad and clip it
    this.fillRectShad = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.fillRectShad.setAttribute('clip-path','url(#clipRectFacOuts_'+this.assps.psId+')');
    this.fillRectShad.setAttribute('style','fill:url(#nodeBgGrad)');
    this.fillRectShad.setAttribute('width','100');
    this.fillRectShad.setAttribute('height','200');
    rectGroup.appendChild(this.fillRectShad);
    
    
    
    // Add bevel shading
    this.bevelShader = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.bevelShader.setAttribute('clip-path','url(#clipRectFacOuts_'+this.assps.psId+')');
    this.bevelShader.setAttribute('style','fill:url(#outBevelGrad)');
    this.bevelShader.setAttribute('width','100');
    this.bevelShader.setAttribute('height','200');
    rectGroup.appendChild(this.bevelShader);
    
    // Add node's wave effect
    this.wave = document.createElementNS('http://www.w3.org/2000/svg','path');
    this.wave.setAttribute('id','nodeWavePath');
    this.wave.setAttribute('d','m -26.75085,102.36218 c 0,0 81.123575,28.30275 186.75085,0 105.62724,-28.302694 187.64436,8.93544 187.64436,8.93544 l 0,-113.4801638 -376.182309,0 z');
    this.wave.setAttribute('style','fill:url(#nodeWaveGrad);stroke:none;');
    this.wave.setAttribute('clip-path','url(#clipRectFacOuts_'+this.assps.psId+')');
    rectGroup.appendChild(this.wave);
    
    // Add icon
    this.outIcon = document.createElementNS('http://www.w3.org/2000/svg','image');
    this.outIcon.setAttribute('x','40');
    this.outIcon.setAttribute('width','25');
    this.outIcon.setAttribute('height','25');
    this.outIcon.setAttributeNS('http://www.w3.org/1999/xlink','xlink:href','../images/icon_out.svg');
    this.nodeSvg.appendChild(this.outIcon);
    
    
    for(var i in outsArr){
        var newIn = new LSDInput(this,-1,outsArr[i].outId);
        newIn.setText(outsArr[i].outName);
        this.addInput(newIn);
    }
    this.flowInputs();

    
}

LSDFacadeOuts.prototype = {
getSvg:function(){
    return this.nodeSvg;
},
    
scaleHeight:function(height){
    height = Math.max(0,height);
    height += 50;
    
    this.ds.setAttribute('height',height+110);
    
    this.clipRect.setAttribute('height',height);
    
    var shadAdj = (Math.min(200,height)/2)-100;
    
    this.fillRectCol.setAttribute('height',height);
    this.fillRectShad.setAttribute('height',height-shadAdj);
    this.fillRectShad.setAttribute('y',shadAdj);
    
    this.bevelShader.setAttribute('height',height);
    
    this.wave.setAttribute('d','m -26.75085,'+(height-50)+' c 0,0 81.123575,28.30275 186.75085,0 105.62724,-28.302694 187.64436,8.93544 187.64436,8.93544 l 0,'+(-100-height)+' -376.182309,0 z');
    
    this.outIcon.setAttribute('y',height-30);
},
    
setPosition:function(x,y){
    this.x = x;
    this.y = y;
    
    this.nodeSvg.setAttribute('transform','translate('+x+','+y+')');
    
    for(var i in this.inArr){
        this.inArr[i].updateWirePos();
    }
},
    
    
addInput:function(inIn){
    var inSvg = inIn.getSvg();
    this.inArr[inIn.inId] = inIn;
    this.nodeSvg.appendChild(inSvg);
    this.assps.inArr[inIn.inId] = inIn;
},
    
flowInputs:function(){
    var curY = 10;
    
    for(i in this.inArr){
        var tempIn = this.inArr[i];
        tempIn.setPosition(10,curY);
        //tempOut.setAttribute('transform','translate(290,'+curY+')');
        curY += 17;
    }
    
    this.scaleHeight(curY);

}
};


// ************************

// Prototype for Wire
function LSDWire(leftin,rightin,wireId){
    this.left = leftin;
    this.right = rightin;
    
    //this.typeId = typeId;
    this.wireId = wireId;
    
    this.leftX = 0;
    this.leftY = 0;
    this.rightX = 0;
    this.rightY = 0;
    
    
    this.svgPath = document.createElementNS('http://www.w3.org/2000/svg','path');
    this.svgPath.setAttribute('style','stroke:#f90;fill:none;stroke-width:3px');
    this.svgPath.setAttribute('d','M0 0 C100 0 100 200 200 200');
    this.svgPath.setAttribute('pointer-events','none');
    
    
}

LSDWire.prototype = {
    getSvg:function(){
        return this.svgPath;
    },
    
    removeWire:function(patchSpace){
        patchSpace.nodegroup.removeChild(this.svgPath);
        if(this.left)
            this.left.wire = null;
        if(this.right)
            this.right.wire = null;
        if(this.wireId)
            lsdApp.server.unwireNodes(this.wireId);
    },
    
    setLeftPos:function(x,y){
        this.leftX = x;
        this.leftY = y;
        var midX = ((this.rightX - x)/2) + x;

        this.svgPath.setAttribute('d','M'+this.leftX+' '+this.leftY+' '+'C'+midX+' '+this.leftY+' '+midX+' '+this.rightY+' '+this.rightX+' '+this.rightY);
    },
    
    setRightPos:function(x,y){
        this.rightX = x;
        this.rightY = y;
        var midX = x - ((x - this.leftX)/2);
        
        this.svgPath.setAttribute('d','M'+this.leftX+' '+this.leftY+' '+'C'+midX+' '+this.leftY+' '+midX+' '+this.rightY+' '+this.rightX+' '+this.rightY);
    },
    
    colourDefault:function(){
        this.svgPath.setAttribute('style','stroke:#f90;fill:none;stroke-width:3px');
    },
    
    colourGood:function(){
        this.svgPath.setAttribute('style','stroke:#0c0;fill:none;stroke-width:3px');
    },
    
    colourBad:function(){
        this.svgPath.setAttribute('style','stroke:#f00;fill:none;stroke-width:3px');
    },
    
    autoConnect:function(patchSpace,leftId,rightId,leftInt,rightInt){
        if(leftInt==1)
            this.left = patchSpace.facadeIns.outArr[leftId];
        else
            this.left = patchSpace.outArr[leftId];
        this.left.wireArr.push(this);
        
        if(rightInt==1)
            this.right = patchSpace.facadeOuts.inArr[rightId];
        else
            this.right = patchSpace.inArr[rightId];
        this.right.wire = this;
        
        this.left.updateWirePos();
        this.right.updateWirePos();
    }
};

// ************************

// Prototype for In
function LSDInput(parnode,typeId,inId){
    this.parnode = parnode;
    this.typeId = typeId;
    this.inId = inId;
    
    this.wire = null;
    this.x = 0;
    this.y = 0;
    
    this.inSvg = document.createElementNS('http://www.w3.org/2000/svg','g');
    this.inSvg.setAttribute('transform','translate(20,20)');
    //this.inSvg.setAttribute('pointer-events','painted');
    
    this.inSvgJq = $(this.inSvg);
    this.inSvgJq.mousedown(this,this.handleMDown);
    
    this.inText = document.createElementNS('http://www.w3.org/2000/svg','text');
    this.inText.textContent = "Test";
    this.inText.setAttribute('class','lsdNodePlugText');
    this.inText.setAttribute('x','10');
    this.inText.setAttribute('y','5');
    this.inSvg.appendChild(this.inText);
    
    var inCircle = document.createElementNS('http://www.w3.org/2000/svg','circle');
    inCircle.setAttribute('style','stroke:black;stroke-width:1px;');
    inCircle.setAttribute('fill','#77c');
    inCircle.setAttribute('r','5');
    this.inSvg.appendChild(inCircle);
}

LSDInput.prototype = {
    getSvg:function(){
        return this.inSvg;
    },
    
    setPosition:function(x,y){
        this.x = x;
        this.y = y;
    
        this.inSvg.setAttribute('transform','translate('+x+','+y+')');
    },
    
    setText:function(text){
        this.inText.textContent = text;
    },
    
    handleWireOver:function(event){
        if(event.data.parnode.assps){
            var facadeIn = true;
            event.data.patchSpace = event.data.parnode.assps;
        }
        else
            event.data.patchSpace = event.data.parnode.parps;
        
        var wire = event.data.patchSpace.curWire;
        if((wire.left.typeId==event.data.typeId || facadeIn || wire.left.typeId==0) && 
           event.data.wire==null && event.data.parnode!=wire.left.parnode){
            wire.colourGood();
            event.data.patchSpace.tentativeIn = event.data;
        }
        else
            wire.colourBad();
        
    },
    
    handleWireOut:function(event){
        var wire = event.data.patchSpace.curWire;
        wire.colourDefault();
        
        event.data.patchSpace.tentativeIn = null;
    },
    
    
    updateWirePos:function(){
        if(this.wire){
            var x = this.parnode.x + this.x;
            var y = this.parnode.y + this.y;
            
            this.wire.setRightPos(x,y);
        }
    },
    
    handleMDown:function(event){
        event.preventDefault();
        event.stopPropagation();
        
        event.data.inSvgJq.unbind('mousedown',event.data.handleMDown);

        if(event.data.parnode.assps)
            event.data.patchSpace = event.data.parnode.assps;
        else
            event.data.patchSpace = event.data.parnode.parps;
        
        if(event.data.wire){
            var wire = event.data.wire;
            event.data.wire = null;
            
            var doc = $(document);
            doc.mouseup(event.data,event.data.handleMUp);
            doc.mousemove(event.data,event.data.handleMMove);
            
            

            var x = (event.pageX/event.data.patchSpace.scale)+(event.data.patchSpace.x/event.data.patchSpace.scale);
            var y = (event.pageY/event.data.patchSpace.scale)+(event.data.patchSpace.y/event.data.patchSpace.scale);
            
            wire.right = null;
            wire.setRightPos(x,y);
            
            event.data.patchSpace.openInputListeners(wire);
            event.data.patchSpace.tentativeOut = wire.left;
        }
    },
    
    handleMMove:function(event){
        var x = (event.pageX/event.data.patchSpace.scale)+(event.data.patchSpace.x/event.data.patchSpace.scale);
        var y = (event.pageY/event.data.patchSpace.scale)+(event.data.patchSpace.y/event.data.patchSpace.scale);
        
        
        event.data.patchSpace.curWire.setRightPos(x,y);
    },
    
    handleMUp:function(event){
        var doc = $(document);
        doc.unbind('mouseup',event.data.handleMUp);
        doc.unbind('mousemove',event.data.handleMMove);
        event.data.inSvgJq.mousedown(event.data,event.data.handleMDown);
        
        var wire = event.data.patchSpace.curWire;
        
        // Perform Connection or delete wire if no connection made
        if(event.data.patchSpace.tentativeIn){
            var targetIn = event.data.patchSpace.tentativeIn;
            if(targetIn.typeId==event.data.typeId){
                targetIn.wire = wire;
                targetIn.wire.right = targetIn;
                targetIn.wire.colourDefault();
                targetIn.updateWirePos();
                
                // Disconnect wire
                lsdApp.server.unwireNodes(targetIn.wire.wireId);
                
                // Make the RPC connection
                var targetOut = event.data.patchSpace.tentativeOut;
                
                var leftFacade;
                if(targetOut.parnode.assps)
                    leftFacade = 1;
                else
                    leftFacade = 0;
                
                var rightFacade;
                if(targetIn.parnode.assps)
                    rightFacade = 1;
                else
                    rightFacade = 0;
                
                lsdApp.lastAddedWire = targetIn.wire;
                lsdApp.server.wireNodes(leftFacade,targetOut.outId,rightFacade,targetIn.inId,lsdApp.quickWire);
            }
            else{
                wire.removeWire(event.data.patchSpace);
                //event.data.parnode.parps.wireArr.pop();
            }
        }
        else{
            wire.removeWire(event.data.patchSpace);
            //event.data.parnode.parps.wireArr.pop();
        }
        
        event.data.patchSpace.closeInputListeners();
    }
};


// ************************

// Prototype for Out
function LSDOutput(parnode,typeId,outId){
    this.parnode = parnode;
    this.typeId = typeId;
    this.outId = outId;
    
    this.wireArr = new Array();
    this.x = 0;
    this.y = 0;
    
    this.outSvg = document.createElementNS('http://www.w3.org/2000/svg','g');
    
    this.outSvgJq = $(this.outSvg);
    this.outSvgJq.mousedown(this,this.handleMDown);
    
    this.outText = document.createElementNS('http://www.w3.org/2000/svg','text');
    this.outText.textContent = "Test";
    this.outText.setAttribute('class','lsdNodePlugText');
    this.outText.setAttribute('x','-10');
    this.outText.setAttribute('y','5');
    this.outText.setAttribute('text-anchor','end');
    this.outSvg.appendChild(this.outText);
    
    var outCircle = document.createElementNS('http://www.w3.org/2000/svg','circle');
    outCircle.setAttribute('style','stroke:black;stroke-width:1px;fill:#88f;');
    outCircle.setAttribute('fill','url(#outBevelGrid)');
    outCircle.setAttribute('r','5');
    this.outSvg.appendChild(outCircle);
    
    
    
}

LSDOutput.prototype = {
    getSvg:function(){
        return this.outSvg;
    },
    
    setPosition:function(x,y){
        this.x = x;
        this.y = y;
        
        this.outSvg.setAttribute('transform','translate('+x+','+y+')');
        
        this.updateWirePos();
    },
    
    setText:function(text){
        this.outText.textContent = text;
    },
    
    updateWirePos:function(){
        if(this.wireArr.length>0){
            var x = this.parnode.x + this.x;
            var y = this.parnode.y + this.y;
            
            for(i in this.wireArr){
                this.wireArr[i].setLeftPos(x,y);
            }
        }
    },
    
    handleMDown:function(event){
        event.preventDefault();
        event.stopPropagation();
        
        if(event.data.parnode.assps)
            event.data.patchSpace = event.data.parnode.assps;
        else
            event.data.patchSpace = event.data.parnode.parps;
        
        event.data.outSvgJq.unbind('mousedown',event.data.handleMDown);
        
        var doc = $(document);
        doc.mouseup(event.data,event.data.handleMUp);
        doc.mousemove(event.data,event.data.handleMMove);
        
        var newWire = new LSDWire(event.data,null,null);
        event.data.patchSpace.addWire(newWire,0,0,30,30);
        event.data.wireArr.push(newWire);
        event.data.updateWirePos();
        
        var x = (event.pageX/event.data.patchSpace.scale)+(event.data.patchSpace.x/event.data.patchSpace.scale);
        var y = (event.pageY/event.data.patchSpace.scale)+(event.data.patchSpace.y/event.data.patchSpace.scale);

        
        newWire.setRightPos(x,y);
        
        event.data.patchSpace.openInputListeners(newWire);
        event.data.patchSpace.tentativeOut = event.data;
    },
    
    handleMMove:function(event){
        var x = (event.pageX/event.data.patchSpace.scale)+(event.data.patchSpace.x/event.data.patchSpace.scale);
        var y = (event.pageY/event.data.patchSpace.scale)+(event.data.patchSpace.y/event.data.patchSpace.scale);

        
        event.data.patchSpace.curWire.setRightPos(x,y);
        
    },
    
    handleMUp:function(event){
        var doc = $(document);
        doc.unbind('mouseup',event.data.handleMUp);
        doc.unbind('mousemove',event.data.handleMMove);
        event.data.outSvgJq.mousedown(event.data,event.data.handleMDown);
        
        // Perform Connection or delete wire if no connection made
        if(event.data.patchSpace.tentativeIn){
            var targetIn = event.data.patchSpace.tentativeIn;
            if(targetIn.typeId==event.data.typeId || targetIn.parnode.assps || event.data.typeId==0){
                targetIn.wire = event.data.wireArr[event.data.wireArr.length-1];
                targetIn.wire.right = targetIn;
                targetIn.wire.colourDefault();
                targetIn.updateWirePos();
                
                // Make the RPC connection
                var targetOut = event.data.patchSpace.tentativeOut;
                
                var leftFacade;
                if(targetOut.parnode.assps)
                    leftFacade = 1;
                else
                    leftFacade = 0;
                
                var rightFacade;
                if(targetIn.parnode.assps)
                    rightFacade = 1;
                else
                    rightFacade = 0;
                
                lsdApp.lastAddedWire = targetIn.wire;
                lsdApp.server.wireNodes(leftFacade,targetOut.outId,rightFacade,targetIn.inId,lsdApp.quickWire);
                
            }
            else{
                var wire = event.data.wireArr.pop();
                wire.removeWire(event.data.patchSpace);
                event.data.patchSpace.wireArr.pop();
            }
        }
        else{
            var wire = event.data.wireArr.pop();
            wire.removeWire(event.data.patchSpace);
            event.data.patchSpace.wireArr.pop();
        }
        
        event.data.patchSpace.closeInputListeners();
    }
};

// ************************

// Prototype for patch space
function LSDPatchSpace(data){
    this.nodeArr = new Array();
    this.facadeArr = new Array();
    this.wireArr = new Array();
    this.inArr = new Array();
    this.outArr = new Array();
    this.psId = data.psId;
    
    
    // Timeout is set to 1sec when any zooming occurs and
    // will update the zoom value with server on expiration
    this.zoomUpdateTimeout = null;
    
    
    // When the patch space is panned, these values hold relevant state
    this.startPanX = 0;
    this.startPanY = 0;
    this.startMouseX = 0;
    this.startMouseY = 0;
    
    // Current panning values (applied to nodegroup)
    this.x = 0;
    this.y = 0;
    this.scale = 1;

    // When a wire is being dragged, it is referenced here
    // so it may be connected on drop
    this.curWire = null;
    
    // When a wire is being dragged, these are set as the
    // mouse rolls in and out of potential plugs
    this.tentativeIn = null;
    this.tentativeOut = null;
    
    this.divE = $(document.createElement('div'));
    this.divE.addClass('lsdpatchspace');
    this.divE.mousedown(this,this.handleMdown);
    this.divE.bind('mousewheel',this,this.handleMwheel);
    this.divE.bind('DOMMouseScroll',this,this.handleMscroll);
    
    this.divE.empty();
    
    // Zoom slider
    var slideCont = $(document.createElement('div'));
    slideCont.addClass('lsdZoomSlider');
    this.divE.append(slideCont);
    this.zoomSlider = $(document.createElement('div'));
    this.zoomSlider.addClass('lsdActualSlider')
    slideCont.append(this.zoomSlider);
    this.zoomSlider.slider({value:50,start:function(event,ui){event.stopPropagation();}});
    this.zoomSlider.bind('slide',this,this.handleZoomSlide);
    
    
    this.psSvg = document.createElementNS('http://www.w3.org/2000/svg','svg');
    this.psSvg.setAttribute('version','1.1');
    this.psSvg.setAttribute('width','100%');
    this.psSvg.setAttribute('height','100%');
    this.psSvg.setAttribute('xmlns','http://www.w3.org/2000/svg');
    this.psSvg.setAttribute('xmlns:xlink','http://www.w3.org/1999/xlink');
    this.psSvg.appendChild(this.getSvgDefs());
    

    this.divE.append(this.psSvg);
    

    
    this.nodegroup = document.createElementNS('http://www.w3.org/2000/svg','g');
    this.psSvg.appendChild(this.nodegroup);
    

    // BG Grid
    this.bgGrid = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.bgGrid.setAttribute('fill','url(#bgGrid)');
    this.bgGrid.setAttribute('width','105%');
    this.bgGrid.setAttribute('height','105%');
    this.nodegroup.appendChild(this.bgGrid);
    
    // Facade Inputs
    this.facadeIns = new LSDFacadeIns(this,data.facadeIns);
    this.nodegroup.appendChild(this.facadeIns.getSvg());
    
    // Facade Outputs
    this.facadeOuts = new LSDFacadeOuts(this,data.facadeOuts);
    this.nodegroup.appendChild(this.facadeOuts.getSvg());	
    this.divE.droppable({drop:this.handleDrop});
    this.divE.data(this);
    
    //this.facadeIns.procArray(data.facadeIns);
    //this.facadeOuts.procArray(data.facadeOuts);
    
    this.handleNodeResp(this,data);
    this.handleWireResp(this,data);
    
    this.setScale(data.scale,data.x,data.y);
    this.setPan(data.x,data.y);
    
}

LSDPatchSpace.prototype = {
    getJquery:function(){
        return this.divE;
    },
    
    addNode:function(node,x,y){
        var nodeSvg = node.getSvg();
        this.nodegroup.appendChild(nodeSvg);
        node.setPosition(x,y);
        this.nodeArr[node.nodeId] = node;
    },
    
    addFacade:function(node,x,y){
        var nodeSvg = node.getSvg();
        this.nodegroup.appendChild(nodeSvg);
        node.setPosition(x,y);
        this.facadeArr[node.nodeId] = node;
    },
    
    addWire:function(wire){
        var wireSvg = wire.getSvg();
        this.nodegroup.appendChild(wireSvg);
        this.wireArr[wire.wireId] = wire;
    },
    
    openInputListeners:function(wire){
        this.curWire = wire;
        this.tentativeIn = null;
        
        for(var i in this.nodeArr){
            var tempNode = this.nodeArr[i];
            for (var j in tempNode.inArr){
                tempNode.inArr[j].inSvgJq.mouseover(tempNode.inArr[j],tempNode.inArr[j].handleWireOver);
                tempNode.inArr[j].inSvgJq.mouseout(tempNode.inArr[j],tempNode.inArr[j].handleWireOut);
            }
        }
        
        for(var i in this.facadeArr){
            var tempNode = this.facadeArr[i];
            for (var j in tempNode.inArr){
                tempNode.inArr[j].inSvgJq.mouseover(tempNode.inArr[j],tempNode.inArr[j].handleWireOver);
                tempNode.inArr[j].inSvgJq.mouseout(tempNode.inArr[j],tempNode.inArr[j].handleWireOut);
            }
        }
        
        // Facade (Output) inputs
        for(var i in this.facadeOuts.inArr){
            this.facadeOuts.inArr[i].inSvgJq.mouseover(this.facadeOuts.inArr[i],this.facadeOuts.inArr[i].handleWireOver);
            this.facadeOuts.inArr[i].inSvgJq.mouseout(this.facadeOuts.inArr[i],this.facadeOuts.inArr[i].handleWireOut);
        }
    },
    
    closeInputListeners:function(){
        this.tentativeIn = null;
        
        for(var i in this.nodeArr){
            var tempNode = this.nodeArr[i];
            for (var j in tempNode.inArr){
                tempNode.inArr[j].inSvgJq.unbind('mouseover',tempNode.inArr[j].handleWireOver);
                tempNode.inArr[j].inSvgJq.unbind('mouseout',tempNode.inArr[j].handleWireOut);
            }
        }
        
        // Facade (Output) inputs
        for(var i in this.facadeOuts.inArr){
            this.facadeOuts.inArr[i].inSvgJq.unbind('mouseover',this.facadeOuts.inArr[i].handleWireOver);
            this.facadeOuts.inArr[i].inSvgJq.unbind('mouseout',this.facadeOuts.inArr[i].handleWireOut);
        }
    },
    
    openOutputListeners:function(wire){
        this.curWire = wire;
    },
    
    closeOutputListeners:function(){
        
    },
    
    setInputs:function(inputArr){
        for(i in inputArr){
            var tempOut = new LSDOutput(this.facadeIns,-1,inputArr[i].inId);
            tempOut.setText(inputArr[i].inName);
            this.facadeIns.addOutput(tempOut);
            this.facadeIns.flowOutputs();
        }
    },
    
    setOutputs:function(outputArr){
        for(i in outputArr){
            var tempIn = new LSDInput(this.facadeOuts,-1,outputArr[i].outId);
            tempIn.setText(outputArr[i].outName);
            this.facadeOuts.addInput(tempIn);
            this.facadeOuts.flowInputs();
        }
    },
    
    handleDrop:function(event,ui){
        var ps = $(this).data();
        var draggee = ui.draggable;
        
        var viewCenterX = (event.pageX/ps.scale)+(ps.x/ps.scale)-140;
        var viewCenterY = (event.pageY/ps.scale)+(ps.y/ps.scale)-80;
        
        if(draggee.data().facade){
            //alert('Adding facade to '+ps.psId);
            lsdApp.server.addFacade(ps.psId,viewCenterX,viewCenterY,lsdApp.reloadCurView);
        }
        else if(draggee.data().classId){
            lsdApp.server.addNode(ps.psId,draggee.data().classId,viewCenterX,viewCenterY,lsdApp.reloadCurView);
        }
    },
    

    handleNodeResp:function(patchSpace,data){
        
        
        for(var i in data.nodes){
            var newNodeData = data.nodes[i];
            if(newNodeData.nodeId){
                
                if(!patchSpace.nodeArr[newNodeData.nodeId]){
                    var newNode = new LSDNode(patchSpace,newNodeData.nodeId,newNodeData.classObj,
                                              newNodeData.name,newNodeData.colour);
                    //Node ins
                    for(var j in newNodeData.nodeIns){
                        var newInput = new LSDInput(newNode,newNodeData.nodeIns[j].typeId,newNodeData.nodeIns[j].inId);
                        newInput.setText(newNodeData.nodeIns[j].name);
                        newNode.addInput(newInput);
                    }
                    newNode.flowInputs();
                    
                    //Node outs
                    for(var j in newNodeData.nodeOuts){
                        var newOutput = new LSDOutput(newNode,newNodeData.nodeOuts[j].typeId,newNodeData.nodeOuts[j].outId);
                        newOutput.setText(newNodeData.nodeOuts[j].name);
                        newNode.addOutput(newOutput);
                    }
                    newNode.flowOutputs();
                    
                    patchSpace.addNode(newNode,newNodeData.x,newNodeData.y);
                }
            }
            
            else if(newNodeData.facadeId){
                if(!patchSpace.facadeArr[newNodeData.facadeId]){
                    var newNode = new LSDFacadeNode(patchSpace,newNodeData.facadeId,newNodeData.name,newNodeData.colour);
                    //Facade ins
                    for(var j in newNodeData.facadeIns){
                        var newInput = new LSDInput(newNode,newNodeData.facadeIns[j].typeId,newNodeData.facadeIns[j].inId);
                        newInput.setText(newNodeData.facadeIns[j].name);
                        newNode.addInput(newInput);
                    }
                    newNode.flowInputs();
                    
                    //Facade outs
                    for(var j in newNodeData.facadeOuts){
                        var newOutput = new LSDOutput(newNode,newNodeData.facadeOuts[j].typeId,newNodeData.facadeOuts[j].outId);
                        newOutput.setText(newNodeData.facadeOuts[j].name);
                        newNode.addOutput(newOutput);
                    }
                    newNode.flowOutputs();
                    
                    patchSpace.addFacade(newNode,newNodeData.x,newNodeData.y);
                }
            }
            
            
        }
        
    },
    
    handleWireResp:function(patchSpace,data){
        
        for(var i in data.wireArr){
            var newWireData = data.wireArr[i];
            if(!patchSpace.wireArr[newWireData.wireId]){
                var newWire = new LSDWire(newWireData.wireLeft,newWireData.wireRight,newWireData.wireId);
                patchSpace.addWire(newWire);
                newWire.autoConnect(patchSpace,newWireData.wireLeft,newWireData.wireRight,
                                    newWireData.wireLeftInt,newWireData.wireRightInt);
            }
        }
    },
    
    setPan:function(x,y){
        this.x = x;
        this.y = y;
        
        
        var newGridMul = 30/this.scale;
                
        this.nodegroup.setAttribute('transform','translate('+-x+','+-y+') scale('+this.scale+')');
        this.bgGrid.setAttribute('x',Math.floor(x/30)*newGridMul);
        this.bgGrid.setAttribute('y',Math.floor(y/30)*newGridMul);
        this.bgGrid.setAttribute('width',105/this.scale+'%');
        this.bgGrid.setAttribute('height',105/this.scale+'%');
    },
    
    setScale:function(scale,vx,vy){
        var scaleDiff = scale - this.scale;
        this.scale = scale;
        
        var offx = scaleDiff*-vx;
        var offy = scaleDiff*-vy;
        
        this.setPan(this.x-offx,this.y-offy);
        
        clearTimeout(this.zoomUpdateTimeout);
        this.zoomUpdateTimeout = setTimeout("lsdApp.handleZoomTimeout()",1000);
    },
    
    
    handleZoomSlide:function(event,ui){
        var viewCenterX = (window.innerWidth/2/event.data.scale)+(event.data.x/event.data.scale);
        var viewCenterY = (window.innerHeight/2/event.data.scale)+(event.data.y/event.data.scale);
        event.data.setScale(((ui.value/100)+0.5),viewCenterX,viewCenterY);
    },
    
    handleMdown:function(event){
        event.data.startMouseX = event.pageX;
        event.data.startMouseY = event.pageY;
        event.data.startPanX = event.data.x;
        event.data.startPanY = event.data.y;
        event.data.divE.unbind('mousedown',event.data.handleMdown);
        var doc = $(document);
        doc.mousemove(event.data,event.data.handleMmove);
        doc.mouseup(event.data,event.data.handleMup);
    },
    
    handleMmove:function(event){
        var mDiffX = event.data.startMouseX - event.pageX;
        var mDiffY = event.data.startMouseY - event.pageY;
        
        
        event.data.setPan(mDiffX+event.data.startPanX,mDiffY+event.data.startPanY);
    },
    
    handleMup:function(event){
        var doc = $(document);
        doc.unbind('mouseup',event.data.handleMup);
        doc.unbind('mousemove',event.data.handleMmove);
        event.data.divE.mousedown(event.data,event.data.handleMdown);
        
        lsdApp.server.panPatchSpace(event.data.psId,event.data.x,event.data.y,event.data.scale);
    },
    
    // Webkit & co (positive up 120 units wheelDelta)
    handleMwheel:function(event){
        var viewCenterX = (event.pageX/event.data.scale)+(event.data.x/event.data.scale);
        var viewCenterY = (event.pageY/event.data.scale)+(event.data.y/event.data.scale);
        
        var newScale = event.data.scale + (event.originalEvent.wheelDelta*0.001);
        newScale = Math.min(1.5,newScale);
        newScale = Math.max(0.5,newScale);
        
        var sliderScale = (newScale - 0.5)*100;
        event.data.zoomSlider.slider('value',sliderScale);
        
        event.data.setScale(newScale,viewCenterX,viewCenterY);
    },
    
    // Firefox (positive down 1 unit detail)
    handleMscroll:function(event){
        var viewCenterX = (event.pageX/event.data.scale)+(event.data.x/event.data.scale);
        var viewCenterY = (event.pageY/event.data.scale)+(event.data.y/event.data.scale);
        
        var newScale = event.data.scale - (event.originalEvent.detail*0.1);
        newScale = Math.min(1.5,newScale);
        newScale = Math.max(0.5,newScale);
        
        var sliderScale = (newScale - 0.5)*100;
        event.data.zoomSlider.slider('value',sliderScale);
        
        event.data.setScale(newScale,viewCenterX,viewCenterY);
    },
    
    getSvgDefs:function(){
        // Referenced objects are defined here.
        // They shall be generated on patch space construction
        
        // The defs
        var defs = document.createElementNS('http://www.w3.org/2000/svg','defs');

        
        // Node Gloss Wave Grad
        var nodeWaveGrad = document.createElementNS('http://www.w3.org/2000/svg','linearGradient');
        nodeWaveGrad.setAttribute('id','nodeWaveGrad');
        nodeWaveGrad.setAttribute('x2','0%');
        nodeWaveGrad.setAttribute('y2','100%');
        defs.appendChild(nodeWaveGrad);
        
        var nodeWaveGradStop1 = document.createElementNS('http://www.w3.org/2000/svg','stop');
        nodeWaveGradStop1.setAttribute('offset','5%');
        nodeWaveGradStop1.setAttribute('stop-color','#fff');
        nodeWaveGradStop1.setAttribute('stop-opacity','0.6');
        nodeWaveGrad.appendChild(nodeWaveGradStop1);
        var nodeWaveGradStop2 = document.createElementNS('http://www.w3.org/2000/svg','stop');
        nodeWaveGradStop2.setAttribute('offset','95%');
        nodeWaveGradStop2.setAttribute('stop-color','#fff');
        nodeWaveGradStop2.setAttribute('stop-opacity','0.1');
        nodeWaveGrad.appendChild(nodeWaveGradStop2);
        
        // General purpose Radial out bevel grad
        var outBevelGrad = document.createElementNS('http://www.w3.org/2000/svg','radialGradient');
        outBevelGrad.setAttribute('id','outBevelGrad');
        defs.appendChild(outBevelGrad);
        
        var outBevelGradStop1 = document.createElementNS('http://www.w3.org/2000/svg','stop');
        outBevelGradStop1.setAttribute('offset','30%');
        outBevelGradStop1.setAttribute('stop-color','#000');
        outBevelGradStop1.setAttribute('stop-opacity','0.1');
        outBevelGrad.appendChild(outBevelGradStop1);
        var outBevelGradStop2 = document.createElementNS('http://www.w3.org/2000/svg','stop');
        outBevelGradStop2.setAttribute('offset','100%');
        outBevelGradStop2.setAttribute('stop-color','#000');
        outBevelGradStop2.setAttribute('stop-opacity','0.2');
        outBevelGrad.appendChild(outBevelGradStop2);

        
        // General purpost Radial in bevel grad
        var inBevelGrad = document.createElementNS('http://www.w3.org/2000/svg','radialGradient');
        inBevelGrad.setAttribute('id','inBevelGrad');
        defs.appendChild(inBevelGrad);
        
        var inBevelGradStop1 = document.createElementNS('http://www.w3.org/2000/svg','stop');
        inBevelGradStop1.setAttribute('offset','30%');
        inBevelGradStop1.setAttribute('stop-color','#fff');
        inBevelGradStop1.setAttribute('stop-opacity','0.1');
        inBevelGrad.appendChild(inBevelGradStop1);
        var inBevelGradStop2 = document.createElementNS('http://www.w3.org/2000/svg','stop');
        inBevelGradStop2.setAttribute('offset','100%');
        inBevelGradStop2.setAttribute('stop-color','#fff');
        inBevelGradStop2.setAttribute('stop-opacity','0.2');
        inBevelGrad.appendChild(inBevelGradStop2);
        
        
        // Drop Shadow grad
        var dsGrad = document.createElementNS('http://www.w3.org/2000/svg','radialGradient');
        dsGrad.setAttribute('id','dsGrad');
        defs.appendChild(dsGrad);
        
        var dsGradStop1 = document.createElementNS('http://www.w3.org/2000/svg','stop');
        dsGradStop1.setAttribute('offset','50%');
        dsGradStop1.setAttribute('stop-color','#000');
        dsGradStop1.setAttribute('stop-opacity','0.5');
        dsGrad.appendChild(dsGradStop1);
        var dsGradStop2 = document.createElementNS('http://www.w3.org/2000/svg','stop');
        dsGradStop2.setAttribute('offset','100%');
        dsGradStop2.setAttribute('stop-color','#000');
        dsGradStop2.setAttribute('stop-opacity','0.0');
        dsGrad.appendChild(dsGradStop2);
        
        // Background grid pattern
        var bgGrid = document.createElementNS('http://www.w3.org/2000/svg','pattern');
        bgGrid.setAttribute('id','bgGrid');
        bgGrid.setAttribute('width','30');
        bgGrid.setAttribute('height','30');
        bgGrid.setAttribute('patternUnits','userSpaceOnUse');
        var line1 = document.createElementNS('http://www.w3.org/2000/svg','line');
        line1.setAttribute('style','stroke:rgba(200,200,200,0.3);stroke-width:1px;');
        line1.setAttribute('x1','0');
        line1.setAttribute('y1','15');
        line1.setAttribute('x2','30');
        line1.setAttribute('y2','15');
        bgGrid.appendChild(line1);
        var line2 = document.createElementNS('http://www.w3.org/2000/svg','line');
        line2.setAttribute('style','stroke:rgba(200,200,200,0.3);stroke-width:1px;');
        line2.setAttribute('x1','15');
        line2.setAttribute('y1','0');
        line2.setAttribute('x2','15');
        line2.setAttribute('y2','30');
        bgGrid.appendChild(line2);
        
        defs.appendChild(bgGrid);

        // Node background shading
        var fillRectGrad = document.createElementNS('http://www.w3.org/2000/svg','linearGradient');
        fillRectGrad.setAttribute('id','nodeBgGrad');
        fillRectGrad.setAttribute('x2','0%');
        fillRectGrad.setAttribute('y2','100%');
        var fillRectGradStop0 = document.createElementNS('http://www.w3.org/2000/svg','stop');
        fillRectGradStop0.setAttribute('stop-color','#fff');
        fillRectGradStop0.setAttribute('stop-opacity','0.5');
        fillRectGradStop0.setAttribute('offset','30%');
        fillRectGrad.appendChild(fillRectGradStop0);
        var fillRectGradStop1 = document.createElementNS('http://www.w3.org/2000/svg','stop');
        fillRectGradStop1.setAttribute('stop-color','#fff');
        fillRectGradStop1.setAttribute('stop-opacity','0.3');
        fillRectGradStop1.setAttribute('offset','70%');
        fillRectGrad.appendChild(fillRectGradStop1);
        var fillRectGradStop2 = document.createElementNS('http://www.w3.org/2000/svg','stop');
        fillRectGradStop2.setAttribute('stop-color','#000');
        fillRectGradStop2.setAttribute('stop-opacity','0.7');
        fillRectGradStop2.setAttribute('offset','100%');
        fillRectGrad.appendChild(fillRectGradStop2);
        
        defs.appendChild(fillRectGrad);
        
        
        return defs;
    },
    
    giveNodeFocus:function(nodeSvg){
        this.nodegroup.appendChild(nodeSvg);
    }
};


// Dialogue for changing node colour
function LSDDialogueNodeColour(title,colour,cbData,cbDone){
    this.colour = colour;
    this.cbData = cbData;
    this.cbDone = cbDone;
    
    this.dialogue = $(document.createElement('div')).css('overflow','hidden').css('padding','0');
    this.dialogue.dialog({title:title,resizable:false,width:300,height:450,modal:true});
    this.dialogue.bind('dialogclose',this,function(event){event.data.dialogue.remove();
                       event.data.cbDone(event.data.cbData,event.data.colour);});
    
    
    // SVG to draw preview colouring
    this.previewSvg = document.createElementNS('http://www.w3.org/2000/svg','svg');
    this.previewSvg.setAttribute('version','1.1');
    this.previewSvg.setAttribute('width',300);
    this.previewSvg.setAttribute('height',250);
    this.previewSvg.setAttribute('xmlns','http://www.w3.org/2000/svg');
    this.previewSvg.setAttribute('xmlns:xlink','http://www.w3.org/1999/xlink');
    this.previewSvg.appendChild(this.getSvgDefs());
    
    this.dialogue.append(this.previewSvg);
    
    
    // Colour picker to set colour
    var cpPlac = $(document.createElement('div'));
    cpPlac.css('position','absolute');
    cpPlac.css('left',52);
    cpPlac.css('top',200);
    this.cpObj = $.farbtastic(cpPlac,this.handleRGBChange);
    this.dialogue.append(cpPlac);
    
    
    // Node to be used as colour preview
    this.nodeSvg = document.createElementNS('http://www.w3.org/2000/svg','g');
    this.nodeSvg.setAttribute('transform','scale(0.8) translate(38,30)');
    this.previewSvg.appendChild(this.nodeSvg);
    
    // Add Drop Shadow
    this.ds = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.ds.setAttribute('fill','url(#dsGrad_pre)');
    this.ds.setAttribute('x','-100');
    this.ds.setAttribute('y','-80');
    this.ds.setAttribute('width','500');
    this.ds.setAttribute('height','360');
    this.ds.setAttribute('pointer-events','none');
    this.nodeSvg.appendChild(this.ds);
    
    
    // add clip rect path
    var clipPath = document.createElementNS('http://www.w3.org/2000/svg','clipPath');
    clipPath.setAttribute('id','clipRect_pre');
    this.nodeSvg.appendChild(clipPath);
    
    // add rect for above clip path
    this.clipRect = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.clipRect.setAttribute('style','position:absolute;fill:#f55');
    this.clipRect.setAttribute('width','300');
    this.clipRect.setAttribute('height','200');
    this.clipRect.setAttribute('rx','8');
    this.clipRect.setAttribute('ry','8');
    clipPath.appendChild(this.clipRect);
    
    
    
    // Rect content group
    var rectGroup = document.createElementNS('http://www.w3.org/2000/svg','g');
    this.nodeSvg.appendChild(rectGroup);
    
    
    
    // Add fill rect colour and clip it
    this.fillRectCol = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.fillRectCol.setAttribute('clip-path','url(#clipRect_pre)');
    this.fillRectCol.setAttribute('style','fill:#f00');
    this.fillRectCol.setAttribute('width','300');
    this.fillRectCol.setAttribute('height','200');
    rectGroup.appendChild(this.fillRectCol);
    
    // Add fill rect shading and clip it
    this.fillRectShad = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.fillRectShad.setAttribute('clip-path','url(#clipRect_pre)');
    this.fillRectShad.setAttribute('style','fill:url(#nodeBgGrad)');
    this.fillRectShad.setAttribute('width','300');
    this.fillRectShad.setAttribute('height','200');
    rectGroup.appendChild(this.fillRectShad);
    
    // Add bevel shading
    this.bevelShader = document.createElementNS('http://www.w3.org/2000/svg','rect');
    this.bevelShader.setAttribute('clip-path','url(#clipRect_pre)');
    this.bevelShader.setAttribute('style','fill:url(#outBevelGrad)');
    this.bevelShader.setAttribute('width','300');
    this.bevelShader.setAttribute('height','200');
    rectGroup.appendChild(this.bevelShader);
    
    // Add node's wave effect
    this.wave = document.createElementNS('http://www.w3.org/2000/svg','path');
    this.wave.setAttribute('id','nodeWavePath');
    this.wave.setAttribute('d','m -26.75085,150 c 0,0 81.123575,28.30275 186.75085,0 105.62724,-28.302694 187.64436,8.93544 187.64436,8.93544 l 0,-300 -376.182309,0 z');
    this.wave.setAttribute('style','fill:url(#nodeWaveGrad);stroke:none;');
    this.wave.setAttribute('clip-path','url(#clipRect_pre)');
    rectGroup.appendChild(this.wave);

    lsdApp.nodeColourDialogue = this;
    this.cpObj.setRGB([this.colour.r,this.colour.g,this.colour.b]);
    //this.reColourNode(this.colour);
}

LSDDialogueNodeColour.prototype = {
handleRGBChange:function(fader){
    var ncd = lsdApp.nodeColourDialogue;
    ncd.reColourNode({r:ncd.cpObj.rgb[0],g:ncd.cpObj.rgb[1],b:ncd.cpObj.rgb[2]});
},
    
reColourNode:function(rgb){
    this.fillRectCol.setAttribute('style','fill:rgba('+Math.floor(rgb.r*255)+','+
                                  Math.floor(rgb.g*255)+','+Math.floor(rgb.b*255)+',1)');
    this.colour = rgb;
},
    
getSvgDefs:function(){
    // Referenced objects are defined here.
    // They shall be generated on preview dialogue construction
    
    // The defs
    var defs = document.createElementNS('http://www.w3.org/2000/svg','defs');
    
    // Drop Shadow grad
    var dsGrad = document.createElementNS('http://www.w3.org/2000/svg','radialGradient');
    dsGrad.setAttribute('id','dsGrad_pre');
    defs.appendChild(dsGrad);
    
    var dsGradStop1 = document.createElementNS('http://www.w3.org/2000/svg','stop');
    dsGradStop1.setAttribute('offset','50%');
    dsGradStop1.setAttribute('stop-color','#000');
    dsGradStop1.setAttribute('stop-opacity','0.5');
    dsGrad.appendChild(dsGradStop1);
    var dsGradStop2 = document.createElementNS('http://www.w3.org/2000/svg','stop');
    dsGradStop2.setAttribute('offset','100%');
    dsGradStop2.setAttribute('stop-color','#000');
    dsGradStop2.setAttribute('stop-opacity','0.0');
    dsGrad.appendChild(dsGradStop2);
    
    // Node Gloss Wave Grad
    var nodeWaveGrad = document.createElementNS('http://www.w3.org/2000/svg','linearGradient');
    nodeWaveGrad.setAttribute('id','preNodeWaveGrad');
    nodeWaveGrad.setAttribute('x2','0%');
    nodeWaveGrad.setAttribute('y2','100%');
    defs.appendChild(nodeWaveGrad);
    
    var nodeWaveGradStop1 = document.createElementNS('http://www.w3.org/2000/svg','stop');
    nodeWaveGradStop1.setAttribute('offset','5%');
    nodeWaveGradStop1.setAttribute('stop-color','#fff');
    nodeWaveGradStop1.setAttribute('stop-opacity','0.6');
    nodeWaveGrad.appendChild(nodeWaveGradStop1);
    var nodeWaveGradStop2 = document.createElementNS('http://www.w3.org/2000/svg','stop');
    nodeWaveGradStop2.setAttribute('offset','95%');
    nodeWaveGradStop2.setAttribute('stop-color','#fff');
    nodeWaveGradStop2.setAttribute('stop-opacity','0.1');
    nodeWaveGrad.appendChild(nodeWaveGradStop2);
    
    
    // Node background shading
    var fillRectGrad = document.createElementNS('http://www.w3.org/2000/svg','linearGradient');
    fillRectGrad.setAttribute('id','preNodeBgGrad');
    fillRectGrad.setAttribute('x2','0%');
    fillRectGrad.setAttribute('y2','100%');
    var fillRectGradStop0 = document.createElementNS('http://www.w3.org/2000/svg','stop');
    fillRectGradStop0.setAttribute('stop-color','#fff');
    fillRectGradStop0.setAttribute('stop-opacity','0.5');
    fillRectGradStop0.setAttribute('offset','30%');
    fillRectGrad.appendChild(fillRectGradStop0);
    var fillRectGradStop1 = document.createElementNS('http://www.w3.org/2000/svg','stop');
    fillRectGradStop1.setAttribute('stop-color','#fff');
    fillRectGradStop1.setAttribute('stop-opacity','0.3');
    fillRectGradStop1.setAttribute('offset','70%');
    fillRectGrad.appendChild(fillRectGradStop1);
    var fillRectGradStop2 = document.createElementNS('http://www.w3.org/2000/svg','stop');
    fillRectGradStop2.setAttribute('stop-color','#000');
    fillRectGradStop2.setAttribute('stop-opacity','0.7');
    fillRectGradStop2.setAttribute('offset','100%');
    fillRectGrad.appendChild(fillRectGradStop2);
    
    defs.appendChild(fillRectGrad);
    
    
    return defs;
}
};

// Dialogue for generic naming purposes
function LSDDialogueName(title,txtlbl,cbData,doneCB,edittxt){
    this.cbData = cbData;
    this.doneCB = doneCB;
    
    // Name Box
    this.nameBox = $(document.createElement('input'));
    this.nameBox.attr('type','text');
    this.nameBox.css('font-size','0.7em').attr('required','required');
    if(edittxt){
        this.nameBox.val(edittxt);
    }
    this.nameBox.keypress(this,this.enterAlias);
    
    // Cancel Button
    this.cancelButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-close'}}).click(this,this.cancelClick);
    // OK button
    this.okButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-check'}}).click(this,this.okClick);
    var buttonDiv = $(document.createElement('div')).append(this.cancelButton).append(this.okButton).css('position','absolute').css('right','1em').css('bottom','1em');
    
    // Main div
    this.dialogue = $(document.createElement('div')).dialog({autoOpen:false, title:title,
                                                            resizable:false, modal:true,
                                                            position:['center','center']});
    this.dialogue.addClass('patchDialog');
    this.dialogue.append('<h3>'+txtlbl+'</h3>');
    this.dialogue.append(this.nameBox).append(buttonDiv);
    
    this.dialogue.bind('dialogopen',this,function(event){event.data.nameBox.focus();});
    this.dialogue.bind('dialogclose',this.dialogue,function(event){event.data.remove();});
    this.dialogue.dialog('open');
}

LSDDialogueName.prototype = {
    cancelClick:function(event){
        event.data.dialogue.dialog('close');
    },
    okClick:function(event){
        if(event.data.nameBox.val().length<1)
            return;
        event.data.doneCB(event.data.cbData,event.data.nameBox.val());
        
        event.data.dialogue.dialog('close');
    },
    enterAlias:function(event){
        if(event.keyCode==13)
            event.data.okClick(event);
    }
};

// Dialogue for facade plug manipulation
function LSDDialogueFacadeEdit(server,facadeId){
    this.server = server;
    this.facadeId = facadeId;
    
    // List of inputs
    this.inUl = $(document.createElement('ul')).selectable();
    this.inUl.addClass('dialogList');
    
    // Add/remove input buttons
    this.inAddButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-plus'}});
    this.inRemoveButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-minus'}});
    this.inEditButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-pencil'}});
    this.inEditButton.css('right','0px').css('position','absolute');
    var inManipDiv = $(document.createElement('div'));
    inManipDiv.addClass('listManipButtons').css('height','1px');
    inManipDiv.append(this.inAddButton);
    inManipDiv.append(this.inRemoveButton);
    inManipDiv.append(this.inEditButton);
    
    
    // List of outputs
    this.outUl = $(document.createElement('ul')).selectable();
    this.outUl.addClass('dialogList');
    
    
    // Add/remove output buttons
    this.outAddButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-plus'}});
    this.outRemoveButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-minus'}});
    this.outEditButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-pencil'}});
    this.outEditButton.css('right','0px').css('position','absolute');
    var outManipDiv = $(document.createElement('div'));
    outManipDiv.addClass('listManipButtons');
    outManipDiv.append(this.outAddButton);
    outManipDiv.append(this.outRemoveButton);
    outManipDiv.append(this.outEditButton);
    
    
    // Main div
    this.dialogue = $(document.createElement('div')).dialog({title:"Edit Facade Interface",
                                                            resizable:false,modal:true,
                                                            height:420});
    this.dialogue.addClass('patchDialog');
    this.dialogue.append('<h3>Inputs:</h3>');
    this.dialogue.append(this.inUl);
    this.dialogue.append(inManipDiv);
    this.dialogue.append('<h3>Outputs:</h3>');
    this.dialogue.append(this.outUl);
    this.dialogue.append(outManipDiv);
    this.dialogue.bind('dialogclose',this.dialogue,function(event){event.data.remove();
                  lsdApp.reloadCurView();});
    
    // Set appropriate starting state and callbacks
    this.inRemoveButton.button('disable');
    this.inEditButton.button('disable');
    this.outRemoveButton.button('disable');
    this.outEditButton.button('disable');
    
    // Set Event Callbacks
    this.dialogue.bind('dialogopen',this,this.shown);
    this.dialogue.bind('dialogclose',this,this.hidden);
    
    this.inUl.bind('selectablestop',this,this.procInSel);
    this.inUl.dblclick(this,this.inUpd);
    this.inAddButton.click(this,this.inAdd);
    this.inEditButton.click(this,this.inUpd);
    this.inRemoveButton.click(this,this.inDel);
    
    this.outUl.bind('selectablestop',this,this.procOutSel);
    this.outUl.dblclick(this,this.outUpd);
    this.outAddButton.click(this,this.outAdd);
    this.outEditButton.click(this,this.outUpd);
    this.outRemoveButton.click(this,this.outDel);
    
    // Populate
    this.updateFromServer();
    
}

LSDDialogueFacadeEdit.prototype = {
    updateFromServer:function(){
        this.inUl.append($(document.createElement('div')).addClass('activityIndicator'));
        this.server.getFacade(this.facadeId,this.recvFacade);
    },
    
    updateFromServerWrap:function(){
        lsdApp.facadeEditDialogue.updateFromServer();
    },
    
    recvFacade:function(data){
        var fd = lsdApp.facadeEditDialogue;
        fd.inUl.empty();
        fd.outUl.empty();
        
        for(var i in data.facade.facadeIns){
            var anIn = data.facade.facadeIns[i];
            var inItem = $(document.createElement('li')).addClass('ui-widget-content');
            inItem.append(anIn.inName);
            inItem.data({inId:anIn.inId,name:anIn.inName});
            fd.inUl.append(inItem);
        }
        
        for(var i in data.facade.facadeOuts){
            var anOut = data.facade.facadeOuts[i];
            var outItem = $(document.createElement('li')).addClass('ui-widget-content');
            outItem.append(anOut.outName);
            outItem.data({outId:anOut.outId,name:anOut.outName});
            fd.outUl.append(outItem);
        }
    },
    
    procInSel:function(event){
        var selected = $('.ui-selected',event.data.inUl);
        
        if(selected.size()==0){
            event.data.inRemoveButton.button('disable');
            event.data.inEditButton.button('disable');
        }
        else if(selected.size()>1){
            event.data.inRemoveButton.button('enable');
            event.data.inEditButton.button('disable');
        }
        else{
            event.data.inRemoveButton.button('enable');
            event.data.inEditButton.button('enable');
        }
    },
    
    procOutSel:function(event){
        var selected = $('.ui-selected',event.data.outUl);
        
        if(selected.size()==0){
            event.data.outRemoveButton.button('disable');
            event.data.outEditButton.button('disable');
        }
        else if(selected.size()>1){
            event.data.outRemoveButton.button('enable');
            event.data.outEditButton.button('disable');
        }
        else{
            event.data.outRemoveButton.button('enable');
            event.data.outEditButton.button('enable');
        }
    },
    
    inAdd:function(event){
        new LSDDialogueName("Add Facade Input","Input Name:",event.data,event.data.inAddFollow);
    },
    
    inAddFollow:function(data,name){
        data.server.createFacadeIn(name,data.facadeId,data.updateFromServerWrap);
    },
        
    inUpd:function(event){
        var inObj = $('.ui-selected',event.data.inUl).first().data();
        new LSDDialogueName("Edit Facade Input","Input Name:",{fd:event.data,inObj:inObj},
                            event.data.inUpdFollow,inObj.name);
    },
    
    inUpdFollow:function(data,name){
        data.fd.server.updateFacadeIn(data.inObj.inId,name,data.fd.updateFromServerWrap);
    },
        
    inDel:function(event){
        var selected = $('.ui-selected',event.data.inUl);
        selected.each(function(){
                      event.data.server.deleteFacadeIn($(this).data().inId,event.data.updateFromServerWrap);
                      });
    },
        
    outAdd:function(event){
        new LSDDialogueName("Add Facade Output","Output Name:",event.data,event.data.outAddFollow);
    },
    
    outAddFollow:function(data,name){
        data.server.createFacadeOut(name,data.facadeId,data.updateFromServerWrap);
    },
        
    outUpd:function(event){
        var outObj = $('.ui-selected',event.data.outUl).first().data();
        new LSDDialogueName("Edit Facade Output","Output Name:",{fd:event.data,outObj:outObj},
                            event.data.outUpdFollow,outObj.name);
    },
    
    outUpdFollow:function(data,name){
        data.fd.server.updateFacadeOut(data.outObj.outId,name,data.fd.updateFromServerWrap);
    },
        
    outDel:function(event){
        var selected = $('.ui-selected',event.data.outUl);
        selected.each(function(){
                      event.data.server.deleteFacadeOut($(this).data().outId,event.data.updateFromServerWrap);
                      });
    }
    
};


// Stuff for channel patching dialogues below

function LSDConfirmationDialogue(msg,titleMsg,okMsg,okFunc){
    var confirmDia = $(document.createElement('div')).dialog({title:titleMsg, resizable:false, modal:true});
    confirmDia.append('<p><span class="ui-icon ui-icon-alert" style="float:left; margin:0 7px 20px 0;"></span>'+msg+'</p>');
    confirmDia.dialog('option','buttons',[{text:"Cancel", click:function(){$(this).dialog('close');}}/*{text:okMsg,click:function(){okFunc();}}*/]);
}

// Object representing partition in partition list
function LSDDialoguePatchPartitionObj(partId,name){
    this.channelArr = new Array();
    this.partId = partId;
    this.name = name;
    
    this.li = $(document.createElement('li'));
    this.li.addClass('ui-widget-content');
    this.li.data(this);
    
    this.setName(name);
}

LSDDialoguePatchPartitionObj.prototype = {
    setName:function(name){
        this.name = name;
        this.li.text(name);
    },
    
    serverDelete:function(server){
        server.deletePartition(this.partId);
        this.li.remove();
    },
    
    serverUpdate:function(server,name,imageFile){
        server.updatePartition(this.partId,name,imageFile);
        this.setName(name);
    }
};

// Object representing channel in partition
function LSDDialoguePatchChannelObj(chanId,name,single,sixteenBit,rObj,gObj,bObj){
    this.name = name;
    this.chanId = chanId;
    this.single = single;
    this.sixteenBit = sixteenBit;
    this.rObj = rObj;
    this.gObj = gObj;
    this.bObj = bObj;
    
    this.li = $(document.createElement('li'));
    this.li.addClass('ui-widget-content');
    this.li.data(this);
    
    this.setName(name);
}

LSDDialoguePatchChannelObj.prototype = {
    setName:function(name){
        this.name = name;
        this.li.text(name);
    },
    
    serverDelete:function(server){
        server.deleteChannel(this.chanId);
        this.li.remove();
    },
        
    serverUpdate:function(server,name,single,sixteenBit,redAddr,greenAddr,blueAddr){
        server.updateChannel(this.chanId,name,single,sixteenBit,redAddr,greenAddr,blueAddr);
        this.setName(name);
        this.single = single;
        this.sixteenBit = sixteenBit;
        this.rObj = redAddr;
        this.gObj = greenAddr;
        this.bObj = blueAddr;
    }
};

// Dialogue for editing/adding partition
function LSDDialoguePatchPartition(parDialogue,editObj){
    lsdApp.patchPartitionDialogue = this;
    if(editObj)
        this.ppobj = editObj;
    this.parDialogue = parDialogue;
    
    this.imageFile = null;
    
    // Name Box
    this.nameBox = $(document.createElement('input'));
    this.nameBox.attr('type','text');
    this.nameBox.css('font-size','0.7em').attr('required','required');
    if(editObj){
        this.nameBox.val(editObj.name);
    }
    this.nameBox.keypress(this,this.enterAlias);
    
    // File Uploader
    this.fuDiv = document.createElement('div');
    this.fu = new qq.FileUploader({element:this.fuDiv,action:'../imageuploader/imageuploader.php',
                                  onSubmit:function(id, fileName){lsdApp.patchPartitionDialogue.imageFile = fileName;
                                  return true;}});
    
    // Cancel Button
    this.cancelButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-close'}}).click(this,this.cancelClick);
    // OK button
    this.okButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-check'}}).click(this,this.okClick);
    var buttonDiv = $(document.createElement('div')).append(this.cancelButton).append(this.okButton).css('position','absolute').css('right','1em').css('bottom','1em');
    
    // Main div
    this.dialogue = $(document.createElement('div')).dialog({autoOpen:false, title:(this.ppobj)?'Edit Partition':'Create Partition', resizable:false, modal:true, position:['center','center'], height:200});
    this.dialogue.addClass('patchDialog');
    this.dialogue.append('<h3>Partition Name:</h3>');
    this.dialogue.append(this.nameBox).append(this.fuDiv).append(buttonDiv);
    
    this.dialogue.bind('dialogopen',this,function(event){event.data.nameBox.focus();});
    this.dialogue.dialog('open');
}

LSDDialoguePatchPartition.prototype = {
    cancelClick:function(event){
        event.data.dialogue.dialog('close');
    },
    okClick:function(event){
        if(event.data.nameBox.val().length<1)
            return;
        if(event.data.ppobj){
            //new LSDConfirmationDialogue('This will update the server','Confirm Update','Update',function(){});

            event.data.ppobj.serverUpdate(event.data.parDialogue.server,event.data.nameBox.val(),
                                          event.data.imageFile);
        }
        else{
            event.data.parDialogue.server.createPartition(event.data.nameBox.val(),event.data.parDialogue.updateEvent,
                                                          event.data.imageFile);
        }
        
        event.data.dialogue.dialog('close');
    },
    enterAlias:function(event){
        if(event.keyCode==13)
            event.data.okClick(event);
    }
};

// Dialogue for editing/adding channel
function LSDDialoguePatchChannel(parDialogue,partElem,editObj){
    if(editObj)
        this.pcobj = editObj;
    
    this.parDialogue = parDialogue;
    this.partElem = partElem;
    
    // Name Box
    this.nameBox = $(document.createElement('input'));
    this.nameBox.attr('type','text').attr('required','required');
    this.nameBox.css('font-size','0.7em');
    if(editObj)
        this.nameBox.val(editObj.name);
    
    // Colour Type radios
    this.monoOpt = $(document.createElement('input'));
    this.monoOpt.attr('type','radio');
    this.monoOpt.attr('value','mono');
    this.monoOpt.attr('name','co');
    //monoOpt.append('Mono');
    this.monoOpt.change(this,this.handleSingleCheck);


    
    this.rgbOpt = $(document.createElement('input'));
    this.rgbOpt.attr('type','radio');
    this.rgbOpt.attr('value','rgb');
    this.rgbOpt.attr('name','co');
    this.rgbOpt.attr('checked','true');
    //rgbOpt.append('RGB');
    this.rgbOpt.change(this,this.handleSingleCheck);

    
    var colourTypeForm = $(document.createElement('form'));
    colourTypeForm.append(this.monoOpt).append('Mono').append(this.rgbOpt).append('RGB');
    colourTypeForm.css('font-size','small').css('padding-top','1em');
    
    this.sixteenBitBox = $(document.createElement('input'));
    this.sixteenBitBox.attr('type','checkbox');
    this.sixteenBitBox.attr('checked','true');
    var sixteenBitDiv = $(document.createElement('div'));
    sixteenBitDiv.append(this.sixteenBitBox).append('16-bit Channel');
    sixteenBitDiv.css('font-size','small').css('padding-top','1em');
    
    if(editObj){
        if(editObj.sixteenBit)
            this.sixteenBitBox.attr('checked',true);
        else
            this.sixteenBitBox.attr('checked',false);
    }
    
    // Address fields
    var addrNote = $(document.createElement('span'));
    addrNote.append('Univ:<br/>Addr:');
    addrNote.css('font-size','small').css('float','left').css('line-height','1.5em').css('margin-right','0.5em');
    
    this.rAddrUniv = $(document.createElement('input')).addClass('smallInput').attr('pattern','[0-9]?[0-9]?[0-9]').attr('maxlength','3').css('background','#f33').css('color','#fff');
    this.gAddrUniv = $(document.createElement('input')).addClass('smallInput').attr('pattern','[0-9]?[0-9]?[0-9]').attr('maxlength','3').css('background','#0a0').css('color','#fff');
    this.bAddrUniv = $(document.createElement('input')).addClass('smallInput').attr('pattern','[0-9]?[0-9]?[0-9]').attr('maxlength','3').css('background','#33f').css('color','#fff');
    this.rAddr = $(document.createElement('input')).addClass('smallInput').attr('pattern','[0-9]?[0-9]?[0-9]').attr('maxlength','3').css('background','#f33').css('color','#fff');
    this.gAddr = $(document.createElement('input')).addClass('smallInput').attr('pattern','[0-9]?[0-9]?[0-9]').attr('maxlength','3').css('background','#0a0').css('color','#fff');
    this.bAddr = $(document.createElement('input')).addClass('smallInput').attr('pattern','[0-9]?[0-9]?[0-9]').attr('maxlength','3').css('background','#33f').css('color','#fff');
    
    var addrDiv = $(document.createElement('div'));
    addrDiv.css('font-size','small').append(this.rAddrUniv).append(this.gAddrUniv).append(this.bAddrUniv).append('<br/>').append(this.rAddr).append(this.gAddr).append(this.bAddr).css('margin-top','1.5em');
    
    var completeAddrForm = $(document.createElement('div'));
    completeAddrForm.append(addrNote).append(addrDiv);
    
    if(editObj){
        this.rAddrUniv.val(editObj.rObj.univId);
        this.rAddr.val(editObj.rObj.lightAddr);
        
        if(!editObj.single){
            this.gAddrUniv.val(editObj.gObj.univId);
            this.gAddr.val(editObj.gObj.lightAddr);
            
            this.bAddrUniv.val(editObj.bObj.univId);
            this.bAddr.val(editObj.bObj.lightAddr);
        }
    }
    
    // Cancel Button
    this.cancelButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-close'}}).click(this,this.cancelClick);
    // OK button
    this.okButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-check'}}).click(this,this.okClick);
    var buttonDiv = $(document.createElement('div')).append(this.cancelButton).append(this.okButton).css('position','absolute').css('right','1em').css('bottom','1em');
    

    
    // Main div
    this.dialogue = $(document.createElement('div')).dialog({title:(editObj)?'Edit Channel':'Create Channel', resizable:false, modal:true, position:['center','center'], height:340});
    this.dialogue.addClass('patchDialog');
    this.dialogue.append('<h3>Channel Name:</h3>');
    this.dialogue.append(this.nameBox);
    this.dialogue.append('<h3>Colour Type:</h3>');
    this.dialogue.append(colourTypeForm);
    this.dialogue.append('<h3>Other Options:</h3>').append(sixteenBitDiv);
    this.dialogue.append('<h3>Addresses:</h3>').append(completeAddrForm);
    this.dialogue.append('<br/><br/>');
    this.dialogue.append(buttonDiv);
    
    if(editObj){
        if(editObj.single){
            this.makeSingle();
        }
        else{
            this.makeRGB();
        }
    }
    
    this.nameBox.focus();
}

LSDDialoguePatchChannel.prototype = {
    setChannelObj:function(pcobj){
        this.pcobj = pcobj;
    },
    cancelClick:function(event){
        event.data.dialogue.dialog('close');
    },
    okClick:function(event){
        if(event.data.nameBox.val().length<1)
            return;
        var name = event.data.nameBox.val();
        var single = 0;
        if(event.data.monoOpt.attr('checked'))
            single = 1;
        var sixteenBit = 0;
        if(event.data.sixteenBitBox.attr('checked'))
            sixteenBit = 1;
        var rObj = {univId:parseInt(event.data.rAddrUniv.val()),lightAddr:parseInt(event.data.rAddr.val())};
        var gObj = {univId:parseInt(event.data.gAddrUniv.val()),lightAddr:parseInt(event.data.gAddr.val())};
        var bObj = {univId:parseInt(event.data.bAddrUniv.val()),lightAddr:parseInt(event.data.bAddr.val())};
        if(isNaN(rObj.univId) || isNaN(rObj.lightAddr))
            return;
        if(!single){
            if(isNaN(gObj.univId) || isNaN(gObj.lightAddr))
                return;
            if(isNaN(bObj.univId) || isNaN(bObj.lightAddr))
                return;
        }
        if(event.data.pcobj){
            event.data.pcobj.serverUpdate(event.data.parDialogue.server,name,single,sixteenBit,rObj,gObj,bObj);
        }
        else{
            event.data.parDialogue.server.createChannel(event.data.partElem.partId,name,single,sixteenBit,rObj,gObj,bObj,event.data.parDialogue.updateEvent);
        }
        
        event.data.dialogue.dialog('close');
    },
    makeSingle:function(){
        this.rAddrUniv.css('background','#fec').css('color','#000');
        this.rAddr.css('background','#fec').css('color','#000');
        this.gAddrUniv.css('visibility','hidden');
        this.gAddr.css('visibility','hidden');
        this.bAddrUniv.css('visibility','hidden');
        this.bAddr.css('visibility','hidden');
        
        this.monoOpt.attr('checked',true);
        this.rgbOpt.attr('checked',false);
    },
    makeRGB:function(){
        this.rAddrUniv.css('background','#f33').css('color','#fff');
        this.rAddr.css('background','#f33').css('color','#fff');
        this.gAddrUniv.css('visibility','visible');
        this.gAddr.css('visibility','visible');
        this.bAddrUniv.css('visibility','visible');
        this.bAddr.css('visibility','visible');
        
        this.monoOpt.attr('checked',false);
        this.rgbOpt.attr('checked',true);
    },
    handleSingleCheck:function(event){
        //alert(event.data.monoOpt.attr('checked'));
        if(event.data.monoOpt.attr('checked'))
            event.data.makeSingle();
        else
            event.data.makeRGB();
    }
};


// Dialogue for patching
function LSDDialoguePatch(server){
    this.server = server;
    this.timer = null;
    
    // List of partitions
    this.partUl = $(document.createElement('ul')).selectable();
    this.partUl.addClass('dialogList');
    
    // Last selected object information (to restore selection after update)
    this.lastPartIdx = null;
    this.lastChanIdx = null;

    
    
    // Add/remove partition buttons
    this.partAddButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-plus'}});
    this.partRemoveButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-minus'}});
    this.partEditButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-pencil'}});
    this.partEditButton.css('right','0px').css('position','absolute');
    var partManipDiv = $(document.createElement('div'));
    partManipDiv.addClass('listManipButtons').css('height','1px');
    partManipDiv.append(this.partAddButton);
    partManipDiv.append(this.partRemoveButton);
    partManipDiv.append(this.partEditButton);

    
    // List of chans
    this.chanUl = $(document.createElement('ul')).selectable();
    this.chanUl.addClass('dialogList');
    
    // Chan note
    this.chanNote = $(document.createElement('div'));
    this.chanNote.addClass('chanNote');
    this.chanNote.append('Select *one* partition to manipulate channels.');
    this.chanUl.append(this.chanNote);
    
    // Add/remove channel buttons
    this.chanAddButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-plus'}});
    this.chanRemoveButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-circle-minus'}});
    this.chanEditButton = $(document.createElement('button')).button({icons:{primary:'ui-icon-pencil'}});
    this.chanEditButton.css('right','0px').css('position','absolute');
    var chanManipDiv = $(document.createElement('div'));
    chanManipDiv.addClass('listManipButtons');
    chanManipDiv.append(this.chanAddButton);
    chanManipDiv.append(this.chanRemoveButton);
    chanManipDiv.append(this.chanEditButton);

    
    // Main div
    this.dialogue = $(document.createElement('div'));
    this.dialogue.addClass('patchDialog');
    this.dialogue.append('<h3>Partitions:</h3>');
    this.dialogue.append(this.partUl);
    this.dialogue.append(partManipDiv);
    this.dialogue.append('<h3>Channels:</h3>');
    this.dialogue.append(this.chanUl);
    this.dialogue.append(chanManipDiv);
    
    // Set appropriate starting state and callbacks
    this.partRemoveButton.button('disable');
    this.partEditButton.button('disable');
    this.chanAddButton.button('disable');
    this.chanRemoveButton.button('disable');
    this.chanEditButton.button('disable');
    
    // Set Event Callbacks
    this.dialogue.bind('dialogopen',this,this.shown);
    this.dialogue.bind('dialogclose',this,this.hidden);
    
    this.partUl.bind('selectablestop',this,this.procPartSel);
    this.partUl.dblclick(this,this.partUpd);
    this.partAddButton.click(this,this.partAdd);
    this.partEditButton.click(this,this.partUpd);
    this.partRemoveButton.click(this,this.partDel);
    
    this.chanUl.bind('selectablestop',this,this.procChanSel);
    this.chanUl.dblclick(this,this.chanUpd);
    this.chanAddButton.click(this,this.chanAdd);
    this.chanEditButton.click(this,this.chanUpd);
    this.chanRemoveButton.click(this,this.chanDel);
    
    // Populate
    this.updateFromServer();
    
}

LSDDialoguePatch.prototype = {
    updateFromServer:function(){
        this.partUl.append($(document.createElement('div')).addClass('activityIndicator'));
        this.server.getChannelPatch(this.handleUpdateResp);
    },
    
    handleUpdateResp:function(data){
        var patchDialogue = lsdApp.patchDialogue;
        patchDialogue.partUl.empty();
        patchDialogue.chanUl.empty();
        
        for(var i in data.partitions){
            var partitionData = data.partitions[i];
            var tempPartObj = new LSDDialoguePatchPartitionObj(partitionData.partId,partitionData.partName);
            
            for(var j in partitionData.channels){
                var chanData = partitionData.channels[j];
                var tempChanObj = new LSDDialoguePatchChannelObj(chanData.chanId,chanData.chanName,chanData.single,chanData.sixteenBit,chanData.redAddr,chanData.greenAddr,chanData.blueAddr);
                tempPartObj.channelArr.push(tempChanObj);
            }
            patchDialogue.partUl.append(tempPartObj.li);
            if(i==patchDialogue.lastPartIdx)
                tempPartObj.li.addClass('ui-selected');
        }
        
        patchDialogue.procPartSel({data:patchDialogue});
    },
    
    updateEvent:function(){
        lsdApp.patchDialogue.updateFromServer();
    },
    
    shown:function(event){
        //event.data.timer = setInterval("lsdApp.patchDialogue.updateFromServer()",2000);
    },
    
    hidden:function(event){
        //clearInterval(event.data.timer);
    },
    
    procPartSel:function(event){
        var selected = $('.ui-selected',event.data.partUl);
        
        if(selected.size()==0){
            event.data.lastPartIdx = null;
            event.data.partRemoveButton.button('disable');
            event.data.chanAddButton.button('disable');
            event.data.chanRemoveButton.button('disable');
            event.data.chanEditButton.button('disable');
            event.data.partEditButton.button('disable');
            event.data.chanUl.empty();
            event.data.chanUl.append(event.data.chanNote);
            event.data.chanUl.selectable('disable');
        }
        else if(selected.size()>1){
            event.data.lastPartIdx = null;
            event.data.partRemoveButton.button('enable');
            event.data.chanAddButton.button('disable');
            event.data.chanRemoveButton.button('disable');
            event.data.chanEditButton.button('disable');
            event.data.partEditButton.button('disable');
            event.data.chanUl.empty();
            event.data.chanUl.append(event.data.chanNote);
            event.data.chanUl.selectable('disable');
        }
        else{
            event.data.lastPartIdx = selected.index();
            event.data.partRemoveButton.button('enable');
            event.data.chanAddButton.button('enable');
            event.data.chanRemoveButton.button('enable');
            event.data.partEditButton.button('enable');
            event.data.chanUl.empty();
            event.data.chanUl.selectable('enable');
            
            var selectedPart = selected.first().data();
            for(var i in selectedPart.channelArr){
                event.data.chanUl.append(selectedPart.channelArr[i].li);
                selectedPart.channelArr[i].li.data(selectedPart.channelArr[i]);
            }
            event.data.procChanSel(event);

        }
        
    },
    
    procChanSel:function(event){
        var selectedparts = $('.ui-selected',event.data.partUl);
        if(selectedparts.size()!=1)
            return;
        
        var selected = $('.ui-selected',event.data.chanUl);
        
        if(selected.size()==0){
            event.data.chanAddButton.button('enable');
            event.data.chanRemoveButton.button('disable');
            event.data.chanEditButton.button('disable');
        }
        else if(selected.size()>1){
            event.data.chanAddButton.button('enable');
            event.data.chanRemoveButton.button('enable');
            event.data.chanEditButton.button('disable');
        }
        else{
            event.data.chanAddButton.button('enable');
            event.data.chanRemoveButton.button('enable');
            event.data.chanEditButton.button('enable');
        }
    },
    
    partAdd:function(event){
        new LSDDialoguePatchPartition(event.data);
    },
    
    partUpd:function(event){
        var selected = $('.ui-selected',event.data.partUl).first().data();
        if(!selected)
            return;
        new LSDDialoguePatchPartition(event.data,selected);
    },
    
    partDel:function(event){
        var selected = $('.ui-selected',event.data.partUl);
        selected.each(function(){$(this).data().serverDelete(event.data.server);});
        event.data.procPartSel({data:event.data});
    },
    
    chanAdd:function(event){
        var selectedPart = $('.ui-selected',event.data.partUl).first().data();
        if(!selectedPart)
            return;
        new LSDDialoguePatchChannel(event.data,selectedPart);
    },
    
    chanUpd:function(event){
        var selectedPart = $('.ui-selected',event.data.partUl).first().data();
        var selectedChan = $('.ui-selected',event.data.chanUl).first().data();
        if(!selectedChan)
            return;
        new LSDDialoguePatchChannel(event.data,selectedPart,selectedChan);
    },
    
    chanDel:function(event){
        var selected = $('.ui-selected',event.data.chanUl);
        selected.each(function(){$(this).data().serverDelete(event.data.server);});
        event.data.procChanSel({data:event.data});
    }
};


function LSDPluginItem(pluginId,dir,sha,enabled,parDia){
    this.pluginId = pluginId;
    this.dir = dir;
    this.sha = sha;
    this.enabled = enabled;
    this.parDia = parDia;
    
    this.li = $(document.createElement('li')).addClass('ui-widget-content').css('position','relative');
    this.li.css('padding-top','0.25em');
    
    this.checkbox = $(document.createElement('input'));
    this.checkbox.attr('type','checkbox');
    this.checkbox.css('position','absolute');
    this.checkbox.css('left','0.7em');
    this.checkbox.css('top','0.5em');
    if(this.enabled)
        this.checkbox.attr('checked',true);
    else
        this.checkbox.attr('checked',false);
    
    this.checkbox.change(this,this.handleCheckChange);
    this.li.append(this.checkbox);
    
    this.li.append('<h1>'+this.dir+'</h1>');
    this.li.append('<h2>Server Binary SHA1:<br />'+this.sha+'</h2>');
    

}

LSDPluginItem.prototype = {
    handleCheckChange:function(event){
        var plugin = event.data;
        if(plugin.checkbox.attr('checked')){
            plugin.enabled = 1;
            plugin.parDia.server.enablePlugin(plugin.pluginId);
            plugin.parDia.changed = true;
        }
        else{
            plugin.enabled = 0;
            plugin.parDia.server.disablePlugin(plugin.pluginId);
            plugin.parDia.changed = true;
        }
    }
};


// Plugin administration tab
function LSDDialoguePlugins(server){
    this.server = server;
    this.changed = false;
    
    this.dialogue = $(document.createElement('div'));
    
    // Plugin List
    this.pluginUl = $(document.createElement('ul'));
    this.pluginUl.addClass('dialogList').addClass('lsdPluginList');
    this.pluginUl.css('height','300px');
    this.dialogue.append(this.pluginUl);
    
    
    this.updateFromServer();
}

LSDDialoguePlugins.prototype = {
    updateFromServer:function(){
        this.pluginUl.append($(document.createElement('div')).addClass('activityIndicator'));
        this.server.getPlugins(this.handleUpdateResp);
    },
    
    handleUpdateResp:function(data){
        var pluginDialogue = lsdApp.pluginDialogue;
        pluginDialogue.pluginUl.empty();
        
        
        for(var i in data.plugins){
            var pluginData = data.plugins[i];
            var newPluginObj = new LSDPluginItem(pluginData.pluginId,pluginData.pluginDir,pluginData.pluginSha,pluginData.enabled,pluginDialogue);
            
            pluginDialogue.pluginUl.append(newPluginObj.li);
        }
    }
};


// Master Preferences dialogue
function LSDDialoguePreferences(server){
    var dialogue = $(document.createElement('div'));
    dialogue.addClass('patchDialog');
    dialogue.dialog({title:'LightShoppe Settings', modal:true, resizable:false, width:325, position:['center',200]});
    dialogue.css('padding','0');
    dialogue.css('padding-top','.2em');
    dialogue.bind('dialogclose',dialogue,function(event){event.data.remove();
                  if(lsdApp.pluginDialogue.changed){window.location.reload();}
                  lsdApp.reloadCurView();});
    
    // Tabdiv
    var tabdiv = $(document.createElement('div'));
    dialogue.append(tabdiv);
    tabdiv.css('padding-bottom','1.5em');
    tabdiv.css('float','left');
    tabdiv.css('height','100%');
    tabdiv.css('width','316px');
    
    // TabList
    var tablist = $(document.createElement('ul'));
    tabdiv.append(tablist);
    
    // Plugins Tab
    var pluginTabLI = $(document.createElement('li'));
    var pluginTabA = $(document.createElement('a'));
    pluginTabLI.append(pluginTabA);
    pluginTabA.append('Plugins');
    pluginTabA.attr('href','#pluginTab');
    tablist.append(pluginTabLI);
    
    lsdApp.pluginDialogue = new LSDDialoguePlugins(server);
    pluginTabA.click(lsdApp.pluginDialogue,function(event){event.data.updateFromServer();});
    lsdApp.pluginDialogue.dialogue.attr('id','pluginTab');
    tabdiv.append(lsdApp.pluginDialogue.dialogue);

    
    // Patch Tab
    var patchTabLI = $(document.createElement('li'));
    var patchTabA = $(document.createElement('a'));
    patchTabLI.append(patchTabA);
    patchTabA.append('Patch');
    patchTabA.attr('href','#patchTab');
    tablist.append(patchTabLI);
    
    lsdApp.patchDialogue = new LSDDialoguePatch(server);
    patchTabA.click({pd:lsdApp.patchDialogue},function(event){event.data.pd.updateFromServer();});
    lsdApp.patchDialogue.dialogue.attr('id','patchTab');
    tabdiv.append(lsdApp.patchDialogue.dialogue);
    
    
    tabdiv.tabs();
}


// Bootstrap function
$(document).ready(function(){lsdApp = new LSDApp();lsdApp.go();});
