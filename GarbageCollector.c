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


#include <stdio.h>
#include <stdlib.h>

#include <sqlite3.h>


#include "GarbageCollector.h"

#define PREPGC(stmt, num) 	if(sqlite3_prepare_v2(gcdb, stmt, -1, &stmt##_S, NULL)!=SQLITE_OK){lsdgc_reportPrepProblem(num);return -1;}
#define FINALGC(stmt)        sqlite3_finalize(stmt##_S)

static sqlite3* gcdb;


void lsdgc_reportPrepProblem(int problem){
    fprintf(stderr,"GC Prep problem: %d\nDetails: %s\n",problem,sqlite3_errmsg(gcdb));
}

// Array functions below. Do not touch!

static const char GET_NEW_ARRAY_ID[] =
"SELECT arrayId FROM SystemArrayMark ORDER BY arrayId DESC LIMIT 1";
static sqlite3_stmt* GET_NEW_ARRAY_ID_S = NULL;
int lsdgc_getNewArrayId(int* arrIdBind){
    int errcode;
    sqlite3_reset(GET_NEW_ARRAY_ID_S);
    errcode = sqlite3_step(GET_NEW_ARRAY_ID_S);
    
    if(errcode == SQLITE_ROW){
        *arrIdBind = sqlite3_column_int(GET_NEW_ARRAY_ID_S,0) + 1;
        return 0;
    }
    else{
        *arrIdBind = 1;
        return 0;
    }
}

static const char SET_ARR_MARK[] =
"INSERT INTO SystemArrayMark (arrayId, delElem) VALUES (?1,?2)";
static sqlite3_stmt* SET_ARR_MARK_S = NULL;
int lsdgc_setArrMark(int arrayId, int arrIdx){
    int errcode;
    sqlite3_reset(SET_ARR_MARK_S);
    sqlite3_bind_int(SET_ARR_MARK_S,1,arrayId);
    sqlite3_bind_int(SET_ARR_MARK_S,2,arrIdx);
    errcode = sqlite3_step(SET_ARR_MARK_S);
    
    //printf("Set Mark for array %d at %d\n",arrayId,arrIdx);
    
    if(errcode == SQLITE_DONE)
        return 0;
    else
        return -1;
}

static const char UNSET_ARR_MARK[] =
"DELETE FROM SystemArrayMark WHERE arrayId=?1 AND delElem=?2";
static sqlite3_stmt* UNSET_ARR_MARK_S = NULL;
int lsdgc_unsetArrMark(int arrayId, int arrIdx){
    int errcode;
    sqlite3_reset(UNSET_ARR_MARK_S);
    sqlite3_bind_int(UNSET_ARR_MARK_S,1,arrayId);
    sqlite3_bind_int(UNSET_ARR_MARK_S,2,arrIdx);
    errcode = sqlite3_step(UNSET_ARR_MARK_S);
    
    //printf("Unset Mark for array %d at %d\n",arrayId,arrIdx);
    
    if(errcode == SQLITE_DONE)
        return 0;
    else
        return -1;
}

static const char DISCOVER_ARR_MARK[] =
"SELECT (delElem) FROM SystemArrayMark WHERE arrayId=?1 ORDER BY delElem ASC LIMIT 1";

static sqlite3_stmt* DISCOVER_ARR_MARK_S = NULL;

int lsdgc_discoverArrMark(int arrayId, int* arrIdxBind){
    int errcode;
    sqlite3_reset(DISCOVER_ARR_MARK_S);
    sqlite3_bind_int(DISCOVER_ARR_MARK_S,1,arrayId);
    errcode = sqlite3_step(DISCOVER_ARR_MARK_S);
    
    if(errcode == SQLITE_ROW){
        *arrIdxBind = sqlite3_column_int(DISCOVER_ARR_MARK_S,0);
        return 0;
    }
    return -1;
}

static const char REMOVE_ARR_MARKS[] =
"DELETE FROM SystemArrayMark WHERE arrayId=?1";
static sqlite3_stmt* REMOVE_ARR_MARKS_S = NULL;
int lsdgc_removeArrIdMarks(int arrayId){
    int errcode;
    sqlite3_reset(REMOVE_ARR_MARKS_S);
    sqlite3_bind_int(REMOVE_ARR_MARKS_S,1,arrayId);
    errcode = sqlite3_step(REMOVE_ARR_MARKS_S);
    
    if(errcode == SQLITE_DONE)
        return 0;
    else
        return -1;
}

static const char GC_INIT[] =
"CREATE TABLE SystemArrayMark (arrayId INTEGER, delElem INTEGER);\n"
"CREATE INDEX SystemArrayMarkIdx ON SystemArrayMark (arrayId,delElem);\n";

int lsdgc_prepGCOps(){
    
    int rc;
    sqlite3* memory;
    rc = sqlite3_open(":memory:", &memory);
    if(rc == SQLITE_OK){
        gcdb = memory;
        
        char* errMsg = NULL;
        rc = sqlite3_exec(gcdb, GC_INIT, NULL, NULL, &errMsg);
        
        if(errMsg){
            fprintf(stderr, "Error during GC init:\n%s\n", errMsg);
            sqlite3_free(errMsg);
        }
        
        PREPGC(GET_NEW_ARRAY_ID,1);
        PREPGC(SET_ARR_MARK,2);
        PREPGC(UNSET_ARR_MARK,3);
        PREPGC(DISCOVER_ARR_MARK,4);
        PREPGC(REMOVE_ARR_MARKS,5);
        
        return 0;
    }
    fprintf(stderr, "Unable to open empty DB for gc\n");
    return -1;

}

int lsdgc_finalGCOps(){
    FINALGC(GET_NEW_ARRAY_ID);
    FINALGC(SET_ARR_MARK);
    FINALGC(UNSET_ARR_MARK);
    FINALGC(DISCOVER_ARR_MARK);
    FINALGC(REMOVE_ARR_MARKS);
    
    return sqlite3_close(gcdb);
    
    return 0;
}
