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

#include <sqlite3.h> // For constants
#include "../../NodeInstAPI.h"

#include "attackDecay.h"

static struct LSD_ScenePlugin const * animationPlugin;


// Stmt bindings
static const char ATTACK_DECAY[] = "AttackDecay";
static unsigned int selectAttackDecay;
static unsigned int insertAttackDecay;
static unsigned int updateAttackDecay_attack;
static unsigned int updateAttackDecay_decay;
static unsigned int deleteAttackDecay;


// Attack Decay Stuff
int createAttackDecay(int nodeId, int inId){
    plugindb_reset(animationPlugin, insertAttackDecay);
    plugindb_bind_int(animationPlugin, insertAttackDecay, 1, nodeId);
    plugindb_bind_int(animationPlugin, insertAttackDecay, 2, inId);
    if(plugindb_step(animationPlugin, insertAttackDecay) == SQLITE_DONE)
        return 0;
    return -1;
}

int getAttackDecayVals(int nodeId, double* attackBind, double* decayBind){
    plugindb_reset(animationPlugin, selectAttackDecay);
    plugindb_bind_int(animationPlugin, selectAttackDecay, 1, inst->dbId);
    
    if(plugindb_step(animationPlugin, selectAttackDecay) == SQLITE_ROW){
        if(attackBind)
            *attackBind = plugindb_column_double(animationPlugin, selectAttackDecay, 0);
        if(decayBind)
            *decayBind = plugindb_column_double(animationPlugin, selectAttackDecay, 1);
        return 0;
    }
    return -1;
}

int restoreAttackDecay(struct LSD_SceneNodeInst* inst){
    if(!inst)
        return -1;
    
    struct AttackDecayState* castData = (struct AttackDecayState*)inst->data;
    
    plugindb_reset(animationPlugin, selectAttackDecay);
    plugindb_bind_int(animationPlugin, selectAttackDecay, 1, inst->dbId);
    
    if(plugindb_step(animationPlugin, selectAttackDecay) == SQLITE_ROW){
        castData->attackRate = plugindb_column_double(animationPlugin, selectAttackDecay, 0);
        castData->decayRate = plugindb_column_double(animationPlugin, selectAttackDecay, 1);
        castData->lastVal = 0.0;
        gettimeofday(&(castData->lastTv),NULL);
        
        int inId = plugindb_column_int(animationPlugin, selectAttackDecay, 2);
        struct LSD_SceneNodeInput* input;
        plugininst_getInputStruct(inst, &input, inId);
        if(input)
            castData->srcIn = input;
        else
            castData->srcIn = NULL;
        
        return 0;
    }
    
    return -1;
}

int updateAttackDecay_attack(int nodeId, double attackAmt){
    plugindb_reset(animationPlugin, updateAttackDecay_attack);
    plugindb_bind_int(animationPlugin, updateAttackDecay_attack, 1, nodeId);
    plugindb_bind_double(animationPlugin, updateAttackDecay_attack, 2, attackAmt);
    if(plugindb_step(animationPlugin, updateAttackDecay_attack) == SQLITE_DONE){
        struct AttackDecayState* castData = NULL;
        plugin_getInstById(animationPlugin,nodeId,&castData);
        if(castData){
            castData->attackRate = attackAmt;
            return 0
        }
    }
    return -1;
}

int updateAttackDecay_decay(int nodeId, double decayAmt){
    plugindb_reset(animationPlugin, updateAttackDecay_decay);
    plugindb_bind_int(animationPlugin, updateAttackDecay_decay, 1, nodeId);
    plugindb_bind_double(animationPlugin, updateAttackDecay_decay, 2, decayAmt);
    if(plugindb_step(animationPlugin, updateAttackDecay_decay) == SQLITE_DONE){
        struct AttackDecayState* castData = NULL;
        plugin_getInstById(animationPlugin,nodeId,&castData);
        if(castData){
            castData->decayRate = decayAmt;
            return 0
        }
    }
    return -1;
}

int deleteAttackDecay(int nodeId){
    plugindb_reset(animationPlugin, deleteAttackDecay);
    plugindb_bind_int(animationPlugin, deleteAttackDecay, 1, nodeId);
    if(plugindb_step(animationPlugin, deleteAttackDecay) == SQLITE_DONE)
        return 0;
    return -1;
}

// Main db init
int initAnimDB(struct LSD_ScenePlugin const * plugin){
    animationPlugin = plugin;
    
    // Attack Decay stuff
    plugininit_createTable(plugin,ATTACK_DECAY,
                           "nodeId INTEGER PRIMARY KEY, inId INTEGER NOT NULL, "
                           "attackAmt REAL DEFAULT 0.0, decayAmt REAL DEFAULT 0.0");
    
    plugindb_prepSelect(plugin,&selectAttackDecay,ATTACK_DECAY,
                        "attackAmt,decayAmt,inId","nodeId=?1");
    
    plugindb_prepInsert(plugin,&insertAttackDecay,ATTACK_DECAY,
                        "nodeId,inId","?1,?2");
    
    plugindb_prepUpdate(plugin,&updateAttackDecay_attack,ATTACK_DECAY,
                        "attackAmt=?2","nodeId=?1");
    
    plugindb_prepUpdate(plugin,&updateAttackDecay_decay,ATTACK_DECAY,
                        "decayAmt=?2","nodeId=?1");
    
    plugindb_prepDelete(plugin,&deleteAttackDecay,ATTACK_DECAY,
                        "nodeId=?1");
    
}

