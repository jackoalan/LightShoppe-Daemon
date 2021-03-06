/*
 **    This file is part of LightShoppe. Copyright 2011 Jack Andersen
 **
 **    LightShoppe is free software: you can redistribute it
 **    and/or modify it under the terms of the GNU General
 **    Public License as published by the Free Software
 **    Foundation, either version 3 of the License, or (at your
 **    option) any later version.
 **
 **    LightShoppe is distributed in the hope that it will
 **    be useful, but WITHOUT ANY WARRANTY; without even the
 **    implied warranty of MERCHANTABILITY or FITNESS FOR A
 **    PARTICULAR PURPOSE.  See the GNU General Public License
 **    for more details.
 **
 **    You should have received a copy of the GNU General
 **    Public License along with LightShoppe.  If not, see
 **    <http://www.gnu.org/licenses/>.
 **
 **    @author Jack Andersen <jackoalan@gmail.com>
 */

#include "DBOps.h"

#include "Array.h"
#include "DBArr.h"
#include "SceneCore.h"
#include "PluginAPI.h"
#include "PluginLoader.h"
#include "Logging.h"

#include <stdio.h>
#include <string.h>
#include <ltdl.h>

/* Gettext stuff */
#ifndef HW_RVL
#include <libintl.h>
#define _(String) gettext (String)
#else
#define _(String) String
#endif

/* Name of this component for logging */
static const char LOG_COMP[] = "DBOps.c";

/* Forward-decl for statement compilation and cleanup */
int
lsddb_prepStmts ();


int
lsddb_finishStmts ();


int
lsddb_traceInput (struct LSD_SceneNodeInput** ptrToBind,
                  int inputId,
                  int* typeIdBind,
                  int* tracedIn);


int
lsddb_traceOutput (struct LSD_SceneNodeOutput** ptrToBind,
                   int outputId,
                   int* typeIdBind,
                   int* tracedOut);


int
lsddb_resolveClassFromId (struct LSD_SceneNodeClass** ptrToBind, int classId);


int
lsddb_rewireNodes ();


static sqlite3* memdb;

void
destruct_SceneDBStmt (void* dbStmt)
{
    if (dbStmt)
    {
        struct LSD_SceneDBStmt* castStmt = dbStmt;
        if (castStmt->stmt)
            sqlite3_finalize (castStmt->stmt);
    }
}


int
lsddb_emptyDB ()
{
    int rc;
    sqlite3* memory;
    rc = sqlite3_open (":memory:", &memory);
    if (rc == SQLITE_OK)
    {
        memdb = memory;
        if (lsddb_initDB () < 0)
        {
            doLog (ERROR, LOG_COMP, _("There was a problem initing DB."));
            return -1;
        }
        if (lsddb_prepStmts () < 0)
        {
            doLog (ERROR, LOG_COMP, _("There was a problem preparing DB statements while opening DB."));
            return -1;
        }
        return 0;
    }
    doLog (ERROR, LOG_COMP, _("Unable to open empty DB."));
    return -1;
}


int
lsddb_openDB (const char* path)
{
    int rc;

    if (path)
    {
        sqlite3* file;
        sqlite3* memory;
        rc = sqlite3_open (path, &file);

        if (rc == SQLITE_OK)
        {
            rc = sqlite3_open (":memory:", &memory);
            if (rc == SQLITE_OK)
            {

                /* All good on both fronts, start copy */
                sqlite3_backup* db_opener;
                db_opener = sqlite3_backup_init (memory, "main", file, "main");
                sqlite3_backup_step (db_opener, -1);
                sqlite3_backup_finish (db_opener);

                /* Done with file */
                sqlite3_close (file);
                memdb = memory;
                if (lsddb_initDB () < 0)
                {
                    doLog (ERROR, LOG_COMP, _("There was a problem initing DB at open."));
                    return -1;
                }
                if (lsddb_prepStmts () < 0)
                {
                    doLog (ERROR, LOG_COMP, _("There was a problem preparing DB statements while opening DB."));
                    return -1;
                }
                return 0;

            }
            else  /* No Memory DB */
            {
                doLog (ERROR, LOG_COMP, _("Error while opening memory DB: %s"),
                         sqlite3_errmsg (memory));
                sqlite3_close (file);
                return -1;
            }

        }
        else  /* No File DB */
        {
            doLog (ERROR, LOG_COMP, _("Error while opening file DB: %s"),
                     sqlite3_errmsg (file));
            sqlite3_close (file);
            return -1;
        }

    }
    else  /* Path NULL */
    {
        doLog (ERROR, LOG_COMP, _("Error while opening DB: Path not specified."));
        return -1;
    }

    return -2; /* Should not be reached */
}


int
lsddb_saveDB (const char* origPath)
{
    int rc;
    sqlite3* file;

    if (origPath && memdb)
    {
        rc = sqlite3_open (origPath, &file);

        if (rc == SQLITE_OK)  /* File OK */
        {
            sqlite3_backup* dbsaver;
            dbsaver = sqlite3_backup_init (file, "main", memdb, "main");
            sqlite3_backup_step (dbsaver, -1);
            sqlite3_backup_finish (dbsaver);

            sqlite3_close (file);
            return 0;
        }
        else
        {
            doLog (ERROR, LOG_COMP, _("Error while opening file DB for saving:\n%s"),
                     sqlite3_errmsg (file));
            sqlite3_close (file);
            return -1;
        }
    }
    else  /* Path NULL */
    {
        doLog (ERROR, LOG_COMP, _("Error while saving DB: path AND memdb must not be NULL."));
        return -1;
    }

    return -2;
}


int
lsddb_autoSaveDB (const char* origPath)
{
    int rc;
    sqlite3* file;
    char newpath[256];

    int pathlen = strlen (origPath);
    if (pathlen > 250)
    {
        doLog (ERROR, LOG_COMP, _("Error while autosaving DB: pathname too long."));
        return -1;
    }
    strcat (newpath, origPath);
    strcat (newpath, ".auto");

    if (origPath && memdb)
    {
        rc = sqlite3_open (newpath, &file);

        if (rc == SQLITE_OK)  /* File OK */
        {
            sqlite3_backup* dbsaver;
            dbsaver = sqlite3_backup_init (file, "main", memdb, "main");
            sqlite3_backup_step (dbsaver, -1);
            sqlite3_backup_finish (dbsaver);

            sqlite3_close (file);
            return 0;
        }
        else
        {
            doLog (ERROR, LOG_COMP, _("Error while opening file DB for saving:\n%s"),
                     sqlite3_errmsg (file));
            sqlite3_close (file);
            return -1;
        }
    }
    else  /* Path NULL */
    {
        doLog (ERROR, LOG_COMP, _("Error while saving DB: path AND memdb must not be NULL."));
        return -1;
    }

    return -2;
}


int
lsddb_closeDB ()
{
    lsddb_finishStmts ();
    return sqlite3_close (memdb);
}


static const char INIT_QUERIES[] =

/* CREATE: ScenePlugin */
    "CREATE TABLE IF NOT EXISTS ScenePlugin (id INTEGER PRIMARY KEY ASC,"
    "pluginDomain TEXT NOT NULL, pluginSha TEXT NOT NULL,"
    "arrayIdx INTEGER, loaded INTEGER NOT NULL,"
    "enabled INTEGER NOT NULL, seen INTEGER NOT NULL DEFAULT 0);\n"

/* Unload plugin status and reset data structure indicies */
    "UPDATE ScenePlugin SET loaded=0;\n"
    "UPDATE ScenePlugin SET arrayIdx=-1;\n"
    "UPDATE ScenePlugin SET seen=0;\n"

/* CREATE: SceneNodeClass */
    "CREATE TABLE IF NOT EXISTS SceneNodeClass (id INTEGER PRIMARY KEY ASC, pluginId INTEGER NOT NULL, "
    "classIdx INTEGER, "
    "name TEXT NOT NULL, desc TEXT NULL, arrayIdx INTEGER, FOREIGN KEY(pluginId) REFERENCES ScenePlugin(id));\n"

    "CREATE INDEX IF NOT EXISTS SceneNodeClassIdx ON SceneNodeClass (classIdx,name);\n"

/* reset data structure array indicies */
    "UPDATE SceneNodeClass SET arrayIdx=-1;\n"

/* CREATE: ScenePatchSpace */
    "CREATE TABLE IF NOT EXISTS ScenePatchSpace (id INTEGER PRIMARY KEY,"
    "name TEXT, parentPatchSpace INTEGER NOT NULL, posX INTEGER, posY INTEGER, "
    "panX INTEGER DEFAULT 0, panY INTEGER DEFAULT 0, scale REAL DEFAULT 1.0, "
    "colourR REAL DEFAULT 0, colourG REAL DEFAULT 1, colourB REAL DEFAULT 0);\n"

/* CREATE: ScenePatchSpaceWidget */
/* For any patchSpace, this table records up to four nodes which provide
/* a widget. A value of -1 for nodeId means 'no widget' */
    "CREATE TABLE IF NOT EXISTS ScenePatchSpaceWidget (psId INTEGER NOT NULL, "
    "psSlot INTEGER NOT NULL, nodeId INTEGER NOT NULL DEFAULT -1);\n"

    "CREATE INDEX IF NOT EXISTS ScenePatchSpaceWidgetIdx ON ScenePatchSpaceWidget "
    "(psId, psSlot);\n"

/* Reserve PatchSpace 0 for partition facade Nodes. */
/* Note that the parentPatchSpace column is meaningless in
 * this special patchSpace (so -1 will do) */
    "REPLACE INTO ScenePatchSpace (id,name,parentPatchSpace) VALUES (0,'Partition Patch Space',-1);\n"

/* CREATE: SceneNodeInst */
    "CREATE TABLE IF NOT EXISTS SceneNodeInst (id INTEGER PRIMARY KEY,"
    "arrIdx INTEGER, classId INTEGER NOT NULL, "
    "patchSpaceId INTEGER NOT NULL,  name TEXT NULL,"
    "colourR REAL DEFAULT 1, colourG REAL DEFAULT 0, colourB REAL DEFAULT 0,"
    "posX INTEGER DEFAULT 0, posY INTEGER DEFAULT 0);\n"

/* reset data structure indicies */
    "UPDATE SceneNodeInst SET arrIdx=-1;\n"

/* CREATE: ScenePluginTable */
    "CREATE TABLE IF NOT EXISTS ScenePluginTable (pluginId INTEGER NOT NULL, "
    "tableName TEXT NOT NULL, FOREIGN KEY(pluginId) REFERENCES ScenePlugin(id));\n"

/* CREATE: SceneDataType */
    "CREATE TABLE IF NOT EXISTS SceneDataType (id INTEGER PRIMARY KEY,"
    "pluginId INTEGER NOT NULL, name TEXT NOT NULL, desc TEXT NULL, "
    "FOREIGN KEY(pluginId) REFERENCES ScenePlugin(id));\n"

/* CREATE: SceneNodeEdge */
    "CREATE TABLE IF NOT EXISTS SceneNodeEdge (id INTEGER PRIMARY KEY,"
    "srcFacadeInt INTEGER NOT NULL, "
    "srcOut INTEGER NOT NULL, "
    "destFacadeInt INTEGER NOT NULL, "
    "destIn INTEGER NOT NULL, "
    "patchSpaceId INTEGER NOT NULL);\n"

/* CREATE: SceneNodeInstInput */
/* instId references SceneNodeInst if facadeBool 0 */
/* instId references associated ScenePatchSpace if
 * facadeBool 1 */
/* aliasedIn refers to the inward placed input contained
 * inside instId patchSpace */
/* -1 for aliasedIn means 'not internally connected' */
/* typeId is set for facade plugs upon internal connection */
    "CREATE TABLE IF NOT EXISTS SceneNodeInstInput (id INTEGER PRIMARY KEY, instId INTEGER NOT NULL, "
    "instInputIdx INTEGER, typeId INTEGER NOT NULL, facadeBool INTEGER NOT NULL, aliasedIn INTEGER, "
    "name TEXT NOT NULL, desc TEXT NULL, arrIdx INTEGER NULL "
    ");\n"

    "CREATE INDEX IF NOT EXISTS iInIdx ON SceneNodeInstInput (instId, instInputIdx);\n"

/* reset data structure indicies */
    "UPDATE SceneNodeInstInput SET arrIdx=-1;\n"

/* CREATE: SceneNodeInstOutput */
/* instId references SceneNodeInst if facadeBool 0 */
/* instId references associated ScenePatchSpace if
 * facadeBool 1 */
/* aliasedOut refers to the inward placed output contained
 * inside instId patchSpace */
/* -1 for aliasedIn means 'not internally connected' */
/* typeId is set for facade plugs upon internal connection */
    "CREATE TABLE IF NOT EXISTS SceneNodeInstOutput (id INTEGER PRIMARY KEY, instId INTEGER NOT NULL, "
    "instOutputIdx INTEGER, typeId INTEGER NOT NULL, facadeBool INTEGER NOT NULL, aliasedOut INTEGER, "
    "name TEXT NOT NULL, desc TEXT NULL, arrIdx INTEGER NULL, "
    "bfFuncIdx INTEGER NOT NULL, bpFuncIdx INTEGER NOT NULL);\n"

    "CREATE INDEX IF NOT EXISTS iOutIdx ON SceneNodeInstOutput (instId, instOutputIdx);\n"

/* reset data structure indicies */
    "UPDATE SceneNodeInstOutput SET arrIdx=-1;\n"

/* CREATE: SystemPartition */
    "CREATE TABLE IF NOT EXISTS SystemPartition (id INTEGER PRIMARY KEY,"
    "name TEXT, arrayIdx INTEGER,"
    "patchSpaceId INTEGER NOT NULL, imageUrl TEXT);\n"

/* Reset data structure indicies */
    "UPDATE SystemPartition SET arrayIdx=-1;\n"

/* CREATE: SystemChannel */
    "CREATE TABLE IF NOT EXISTS SystemChannel (id INTEGER PRIMARY KEY, "
    "name TEXT NOT NULL, partitionId INTEGER NOT NULL, "
    "partitionElem INTEGER, single INTEGER NOT NULL, arrIdx INTEGER,"
    "rAddrId INTEGER NOT NULL, gAddrId INTEGER,"
    "bAddrId INTEGER, facadeOutId INTEGER, FOREIGN KEY(partitionId) REFERENCES SystemPartition(id));\n"

/* Reset Data structures */
    "UPDATE SystemChannel SET arrIdx=-1;\n"

/* CREATE: OlaAddress */
    "CREATE TABLE IF NOT EXISTS OlaAddress (id INTEGER PRIMARY KEY,"
    "olaUnivId INTEGER NOT NULL, olaLightAddr INTEGER NOT NULL,"
    "olaUnivArrIdx INTEGER, sixteenBit INTEGER NOT NULL);\n"

    "CREATE INDEX IF NOT EXISTS OlaAddrIdx ON OlaAddress"
    "(olaUnivId ASC);\n"

/* Reset data structure indicies */
    "UPDATE OlaAddress SET olaUnivArrIdx=-1;\n";

int
lsddb_initDB ()
{
    /* printf("%s\n", INIT_QUERIES); */

    char* errMsg = NULL;
    int rc;
    rc = sqlite3_exec (memdb, INIT_QUERIES, NULL, NULL, &errMsg);

    if (errMsg)
    {
        doLog (ERROR, LOG_COMP, _("Error during DB init:\n%s"), errMsg);
        sqlite3_free (errMsg);
    }
    return rc;
}


/* List of statements to be ran before array restructuring */
static const char RESET_DB[] =
/* Unload plugin status and reset data structure indicies */
    "UPDATE ScenePlugin SET loaded=0;\n"
    "UPDATE ScenePlugin SET arrayIdx=-1;\n"
    "UPDATE ScenePlugin SET seen=0;\n"

/* reset data structure array indicies */
    "UPDATE SceneNodeClass SET arrayIdx=-1;\n"

/* reset data structure indicies */
    "UPDATE SceneNodeInst SET arrIdx=-1;\n"

/* reset data structure indicies */
    "UPDATE SceneNodeInstInput SET arrIdx=-1;\n"

/* reset data structure indicies */
    "UPDATE SceneNodeInstOutput SET arrIdx=-1;\n"

/* Reset data structure indicies */
    "UPDATE SystemPartition SET arrayIdx=-1;\n"

/* Reset Data structures */
    "UPDATE SystemChannel SET arrIdx=-1;\n"

/* Reset data structure indicies */
    "UPDATE OlaAddress SET olaUnivArrIdx=-1;\n";

int
lsddb_resetDB ()
{
    /* printf("%s\n", INIT_QUERIES); */

    char* errMsg = NULL;
    int rc;
    rc = sqlite3_exec (memdb, RESET_DB, NULL, NULL, &errMsg);

    if (errMsg)
    {
        doLog (ERROR, LOG_COMP, _("Error during DB reset:\n%s"), errMsg);
        sqlite3_free (errMsg);
    }
    return rc;

}


static const char CREATE_PATCH_SPACE[] =
    "INSERT INTO ScenePatchSpace (name,parentPatchSpace) VALUES (?1,?2)";
static sqlite3_stmt* CREATE_PATCH_SPACE_S;

int
lsddb_createPatchSpace (const char* name, int* idBinding, int parentPatchSpace)
{

    sqlite3_reset (CREATE_PATCH_SPACE_S);
    if (name)
        sqlite3_bind_text (CREATE_PATCH_SPACE_S, 1, name, -1, NULL);
    sqlite3_bind_int (CREATE_PATCH_SPACE_S, 2, parentPatchSpace);
    if (sqlite3_step (CREATE_PATCH_SPACE_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("There was an error executing initial DB insertion for createPatchSpace()."));
        return -1;
    }

    int patchSpaceId = sqlite3_last_insert_rowid (memdb);

    if (idBinding)
        *idBinding = patchSpaceId;

    return 0;
}


static const char REMOVE_PATCH_SPACE[] =
    "DELETE FROM ScenePatchSpace WHERE id=?1";
static sqlite3_stmt* REMOVE_PATCH_SPACE_S;

static const char REMOVE_PATCH_SPACE_NODES[] =
    "SELECT id FROM SceneNodeInst WHERE patchSpaceId=?1";
static sqlite3_stmt* REMOVE_PATCH_SPACE_NODES_S;

static const char REMOVE_PATCH_SPACE_FACADES[] =
    "SELECT id FROM ScenePatchSpace WHERE parentPatchSpace=?1";
static sqlite3_stmt* REMOVE_PATCH_SPACE_FACADES_S;

static const char REMOVE_PATCH_SPACE_INS[] =
    "SELECT id FROM SceneNodeInstInput WHERE instId=?1 AND facadeBool=1";
static sqlite3_stmt* REMOVE_PATCH_SPACE_INS_S;

static const char REMOVE_PATCH_SPACE_OUTS[] =
    "SELECT id FROM SceneNodeInstOutput WHERE instId=?1 AND facadeBool=1";
static sqlite3_stmt* REMOVE_PATCH_SPACE_OUTS_S;

int
lsddb_removePatchSpace (int psid)
{
    /* remove nodes */
    sqlite3_reset (REMOVE_PATCH_SPACE_NODES_S);
    sqlite3_bind_int (REMOVE_PATCH_SPACE_NODES_S, 1, psid);
    while (sqlite3_step (REMOVE_PATCH_SPACE_NODES_S) == SQLITE_ROW)
    {
        int nodeId = sqlite3_column_int (REMOVE_PATCH_SPACE_NODES_S, 0);
        lsddb_removeNodeInst (nodeId);
    }

    /* remove children patch spaces */
    sqlite3_reset (REMOVE_PATCH_SPACE_FACADES_S);
    sqlite3_bind_int (REMOVE_PATCH_SPACE_FACADES_S, 1, psid);
    while (sqlite3_step (REMOVE_PATCH_SPACE_FACADES_S) == SQLITE_ROW)
    {
        int cpsId = sqlite3_column_int (REMOVE_PATCH_SPACE_FACADES_S, 0);
        lsddb_removePatchSpace (cpsId);
    }

    /* remove ins */
    sqlite3_reset (REMOVE_PATCH_SPACE_INS_S);
    sqlite3_bind_int (REMOVE_PATCH_SPACE_INS_S, 1, psid);
    while (sqlite3_step (REMOVE_PATCH_SPACE_INS_S) == SQLITE_ROW)
    {
        int inId = sqlite3_column_int (REMOVE_PATCH_SPACE_INS_S, 0);
        lsddb_removePatchSpaceIn (inId);
    }

    /* remove outs */
    sqlite3_reset (REMOVE_PATCH_SPACE_OUTS_S);
    sqlite3_bind_int (REMOVE_PATCH_SPACE_OUTS_S, 1, psid);
    while (sqlite3_step (REMOVE_PATCH_SPACE_OUTS_S) == SQLITE_ROW)
    {
        int outId = sqlite3_column_int (REMOVE_PATCH_SPACE_OUTS_S, 0);
        lsddb_removePatchSpaceOut (outId);
    }

    /* Actually delete patch space */
    sqlite3_reset (REMOVE_PATCH_SPACE_S);
    sqlite3_bind_int (REMOVE_PATCH_SPACE_S, 1, psid);
    if (sqlite3_step (REMOVE_PATCH_SPACE_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Error while removing patch space from DB.\nDetails: %s"), sqlite3_errmsg (memdb));
        return -1;
    }

    return 0;
}


static const char UPDATE_PATCH_SPACE_NAME[] =
    "UPDATE ScenePatchSpace SET name=?2 WHERE id=?1";
static sqlite3_stmt* UPDATE_PATCH_SPACE_NAME_S;

int
lsddb_updatePatchSpaceName (int psId, const char* name)
{
    if (!name)
        return -1;

    sqlite3_reset (UPDATE_PATCH_SPACE_NAME_S);
    sqlite3_bind_int (UPDATE_PATCH_SPACE_NAME_S, 1, psId);
    sqlite3_bind_text (UPDATE_PATCH_SPACE_NAME_S, 2, name, -1, NULL);

    if (sqlite3_step (UPDATE_PATCH_SPACE_NAME_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Error while updating patch space name."));
        return -1;
    }

    return 0;
}


/* Update patch space colour */
static const char SET_PS_COLOUR[] =
    "UPDATE ScenePatchSpace SET colourR=?2,colourG=?3,colourB=?4 WHERE id=?1";
static sqlite3_stmt* SET_PS_COLOUR_S;

int
lsddb_setPatchSpaceColour (int psId, cJSON* colour)
{
    if (!colour || colour->type != cJSON_Object)
        return -1;

    cJSON* rObj = cJSON_GetObjectItem (colour, "r");
    cJSON* gObj = cJSON_GetObjectItem (colour, "g");
    cJSON* bObj = cJSON_GetObjectItem (colour, "b");

    if (!rObj || rObj->type != cJSON_Number ||
        !gObj || gObj->type != cJSON_Number ||
        !bObj || bObj->type != cJSON_Number)
        return -1;

    sqlite3_reset (SET_PS_COLOUR_S);
    sqlite3_bind_int (SET_PS_COLOUR_S, 1, psId);
    sqlite3_bind_double (SET_PS_COLOUR_S, 2, rObj->valuedouble);
    sqlite3_bind_double (SET_PS_COLOUR_S, 3, gObj->valuedouble);
    sqlite3_bind_double (SET_PS_COLOUR_S, 4, bObj->valuedouble);

    if (sqlite3_step (SET_PS_COLOUR_S) != SQLITE_DONE)
        return -1;
    return 0;
}


static const char CREATE_PATCH_SPACE_IN[] =
    "INSERT INTO SceneNodeInstInput (instId,typeId,facadeBool,name,arrIdx) VALUES (?1,-1,1,?2,-1)";
static sqlite3_stmt* CREATE_PATCH_SPACE_IN_S;
int
lsddb_createPatchSpaceIn (int patchSpaceId, const char* name, int* idBinding)
{
    sqlite3_reset (CREATE_PATCH_SPACE_IN_S);

    sqlite3_bind_int (CREATE_PATCH_SPACE_IN_S, 1, patchSpaceId);
    sqlite3_bind_text (CREATE_PATCH_SPACE_IN_S, 2, name, -1, NULL);

    if (sqlite3_step (CREATE_PATCH_SPACE_IN_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Unable to insert Patch Space Input in createPatchSpaceIn()."));
        return -1;
    }

    if (idBinding)
        *idBinding = sqlite3_last_insert_rowid (memdb);

    return 0;
}


static const char CREATE_PATCH_SPACE_OUT[] =
    "INSERT INTO SceneNodeInstOutput (instId,typeId,facadeBool,name,arrIdx,bfFuncIdx,bpFuncIdx) VALUES (?1,-1,1,?2,-1,-1,-1)";
static sqlite3_stmt* CREATE_PATCH_SPACE_OUT_S;

int
lsddb_createPatchSpaceOut (int patchSpaceId, const char* name, int* idBinding)
{
    sqlite3_reset (CREATE_PATCH_SPACE_OUT_S);

    sqlite3_bind_int (CREATE_PATCH_SPACE_OUT_S, 1, patchSpaceId);
    sqlite3_bind_text (CREATE_PATCH_SPACE_OUT_S, 2, name, -1, NULL);

    if (sqlite3_step (CREATE_PATCH_SPACE_OUT_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Unable to insert Patch Space Output in createPatchSpaceOut()\nDetails: %s"), 
               sqlite3_errmsg (memdb));

        return -1;
    }

    if (idBinding)
        *idBinding = sqlite3_last_insert_rowid (memdb);

    return 0;
}


static const char REMOVE_PATCH_SPACE_OUT[] =
    "DELETE FROM SceneNodeInstOutput WHERE id=?1 AND facadeBool=1";
static sqlite3_stmt* REMOVE_PATCH_SPACE_OUT_S;

static const char REMOVE_PATCH_SPACE_IN[] =
    "DELETE FROM SceneNodeInstInput WHERE id=?1 AND facadeBool=1";
static sqlite3_stmt* REMOVE_PATCH_SPACE_IN_S;

int
lsddb_removePatchSpaceOut (int outId)
{
    sqlite3_reset (REMOVE_PATCH_SPACE_OUT_S);
    sqlite3_bind_int (REMOVE_PATCH_SPACE_OUT_S, 1, outId);
    if (sqlite3_step (REMOVE_PATCH_SPACE_OUT_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Error while removing patch space output."));
        return -1;
    }
    return 0;
}


int
lsddb_removePatchSpaceIn (int inId)
{
    sqlite3_reset (REMOVE_PATCH_SPACE_IN_S);
    sqlite3_bind_int (REMOVE_PATCH_SPACE_IN_S, 1, inId);
    if (sqlite3_step (REMOVE_PATCH_SPACE_IN_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Error while removing patch space input."));
        return -1;
    }
    return 0;
}


static const char UPDATE_PATCH_SPACE_IN_NAME[] =
    "UPDATE SceneNodeInstInput SET name=?2 WHERE id=?1 AND facadeBool=1";
static sqlite3_stmt* UPDATE_PATCH_SPACE_IN_NAME_S;

static const char UPDATE_PATCH_SPACE_OUT_NAME[] =
    "UPDATE SceneNodeInstOutput SET name=?2 WHERE id=?1 AND facadeBool=1";
static sqlite3_stmt* UPDATE_PATCH_SPACE_OUT_NAME_S;

int
lsddb_updatePatchSpaceInName (int inId, const char* name)
{
    if (!name)
        return -1;

    sqlite3_reset (UPDATE_PATCH_SPACE_IN_NAME_S);
    sqlite3_bind_int (UPDATE_PATCH_SPACE_IN_NAME_S, 1, inId);
    sqlite3_bind_text (UPDATE_PATCH_SPACE_IN_NAME_S, 2, name, -1, NULL);

    if (sqlite3_step (UPDATE_PATCH_SPACE_IN_NAME_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Unable to update PatchSpace input name."));
        return -1;
    }

    return 0;
}


int
lsddb_updatePatchSpaceOutName (int outId, const char* name)
{
    if (!name)
        return -1;

    sqlite3_reset (UPDATE_PATCH_SPACE_OUT_NAME_S);
    sqlite3_bind_int (UPDATE_PATCH_SPACE_OUT_NAME_S, 1, outId);
    sqlite3_bind_text (UPDATE_PATCH_SPACE_OUT_NAME_S, 2, name, -1, NULL);

    if (sqlite3_step (UPDATE_PATCH_SPACE_OUT_NAME_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Unable to update PatchSpace output name."));
        return -1;
    }

    return 0;
}


static const char CREATE_PARTITION[] =
    "INSERT INTO SystemPartition (name,patchSpaceId) VALUES (?1,?2)";
static sqlite3_stmt* CREATE_PARTITION_S;

int
lsddb_createPartition (const char* name, int* idBinding)
{

    if (!name)
    {
        doLog (ERROR, LOG_COMP, _("Name must not be NULL in createPartition()."));
        return -1;
    }
    if (strlen (name) > 200)
    {
        doLog (ERROR, LOG_COMP, _("Name is too long in createPartition()."));
        return -1;
    }


    /* This no longer happens because the partition state is
     * only constructed once at startup. It's left here as
     * reference of what will happen iteratively once for
     * every partition present in the database at start time
      *
      * size_t partArrIdx;
      * struct LSD_Partition* partArrPtr;
      * if(insertElem(getArr_lsdPartitionArr(),&partArrIdx,(void**)&partArrPtr)<0){
      *  fprintf(stderr,"Unable to insert Partition into
      * array in createPartition()\n");
      *  return -1;
      * }*/

    /* Create Partition's PatchSpace */
    /* char psName[256]; */
    /* memset(psName,0,256); */
    /* strcat(psName,name); */
    /* strcat(psName,"_patch_space"); */
    int psId;
    if (lsddb_createPatchSpace (name, &psId, 0) < 0)
    {
        doLog (ERROR, LOG_COMP, _("createPatchSpace failed in createPartition()."));
        return -1;
    }

    /* Create partition */
    sqlite3_reset (CREATE_PARTITION_S);
    sqlite3_bind_text (CREATE_PARTITION_S, 1, name, -1, NULL);
    sqlite3_bind_int (CREATE_PARTITION_S, 2, psId);

    if (sqlite3_step (CREATE_PARTITION_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Error inserting partition into DB in createPartition()."));
        return -1;
    }

    int partId;
    partId = sqlite3_last_insert_rowid (memdb);

    if (idBinding)
        *idBinding = partId;

    return 0;
}


static const char SET_PARTITION_IMAGE[] =
    "UPDATE SystemPartition SET imageUrl=?2 WHERE id=?1";
static sqlite3_stmt* SET_PARTITION_IMAGE_S;

int
lsddb_updatePartitionImage (int partId, const char* imageUrl)
{
    if (!imageUrl)
        return -1;

    sqlite3_reset (SET_PARTITION_IMAGE_S);
    sqlite3_bind_int (SET_PARTITION_IMAGE_S, 1, partId);
    sqlite3_bind_text (SET_PARTITION_IMAGE_S, 2, imageUrl, -1, NULL);

    if (sqlite3_step (SET_PARTITION_IMAGE_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Unable to set image in setPartitionImage()\nDetails: %s"), 
                                  sqlite3_errmsg (memdb));
        return -1;
    }

    return 0;
}


static const char REMOVE_PARTITON[] =
    "DELETE FROM SystemPartition WHERE id=?1";
static sqlite3_stmt* REMOVE_PARTITON_S;

static const char GET_PARTITON_PATCHSPACE[] =
    "SELECT patchSpaceId FROM SystemPartition WHERE id=?1";
static sqlite3_stmt* GET_PARTITON_PATCHSPACE_S;

static const char REMOVE_PARTITON_GET_CHANNELS[] =
    "SELECT id FROM SystemChannel WHERE partitionId=?1";
static sqlite3_stmt* REMOVE_PARTITON_GET_CHANNELS_S;

int
lsddb_removePartition (int partId)
{
    /* Remove partition's patchSpace */
    sqlite3_reset (GET_PARTITON_PATCHSPACE_S);
    sqlite3_bind_int (GET_PARTITON_PATCHSPACE_S, 1, partId);

    int psId;
    if (sqlite3_step (GET_PARTITON_PATCHSPACE_S) == SQLITE_ROW)
        psId = sqlite3_column_int (GET_PARTITON_PATCHSPACE_S, 0);
    else
    {
        doLog (ERROR, LOG_COMP, _("Requested partition to be removed does not exist."));
        return -1;
    }

    if (lsddb_removePatchSpace (psId) < 0)
        doLog (ERROR, LOG_COMP, _("There was a problem while removing partition's patchSpace in removePartition()."));

    /* Remove partition's channels */
    sqlite3_reset (REMOVE_PARTITON_GET_CHANNELS_S);
    sqlite3_bind_int (REMOVE_PARTITON_GET_CHANNELS_S, 1, partId);
    while (sqlite3_step (REMOVE_PARTITON_GET_CHANNELS_S) == SQLITE_ROW)
    {
        int chanId = sqlite3_column_int (REMOVE_PARTITON_GET_CHANNELS_S, 0);
        lsddb_deletePatchChannel (chanId);
    }

    /* remove partition record */
    sqlite3_reset (REMOVE_PARTITON_S);
    sqlite3_bind_int (REMOVE_PARTITON_S, 1, partId);

    if (sqlite3_step (REMOVE_PARTITON_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Problem while removing partition from DB in removePartition()\nDetails: %s"), 
               sqlite3_errmsg (memdb));
        return -1;
    }

    return 0;
}


static const char UPDATE_PARTITON_NAME[] =
    "UPDATE SystemPartition SET name=?2 WHERE id=?1";
static sqlite3_stmt* UPDATE_PARTITON_NAME_S;

static const char UPDATE_PARTITON_PS_NAME[] =
    "UPDATE ScenePatchSpace SET name=?2 WHERE id=?1";
static sqlite3_stmt* UPDATE_PARTITON_PS_NAME_S;

int
lsddb_updatePartitionName (int partId, const char* name)
{
    if (!name)
        return -1;

    sqlite3_reset (UPDATE_PARTITON_NAME_S);
    sqlite3_bind_int (UPDATE_PARTITON_NAME_S, 1, partId);
    sqlite3_bind_text (UPDATE_PARTITON_NAME_S, 2, name, -1, NULL);

    if (sqlite3_step (UPDATE_PARTITON_NAME_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Ubable to Update Name in updatePartitionName()."));
        return -1;
    }

    /* Update Partition's patch space name as well */
    int psId;
    sqlite3_reset (GET_PARTITON_PATCHSPACE_S);
    sqlite3_bind_int (GET_PARTITON_PATCHSPACE_S, 1, partId);
    if (sqlite3_step (GET_PARTITON_PATCHSPACE_S) == SQLITE_ROW)
        psId = sqlite3_column_int (GET_PARTITON_PATCHSPACE_S, 0);
    else
    {
        doLog (ERROR, LOG_COMP, _("Unable to get psId in updatePartitionName()."));
        return -1;
    }

    sqlite3_reset (UPDATE_PARTITON_PS_NAME_S);
    sqlite3_bind_int (UPDATE_PARTITON_PS_NAME_S, 1, psId);
    sqlite3_bind_text (UPDATE_PARTITON_PS_NAME_S, 2, name, -1, NULL);
    if (sqlite3_step (UPDATE_PARTITON_PS_NAME_S) != SQLITE_DONE)
        doLog (ERROR, LOG_COMP, _("Unable to update patchSpace's name in updatePartitionName()."));

    return 0;
}


/* Registers class in DB (or ignores if name string already
 * exists for the plugin in question) */
/* Will then struct the class and provide its pointer */
static const char ADD_NODE_CLASS_CHECK[] =
    "SELECT arrayIdx,id FROM SceneNodeClass WHERE pluginId=?1 AND name=?2";
static sqlite3_stmt* ADD_NODE_CLASS_CHECK_S;

static const char ADD_NODE_CLASS_INSERT[] =
    "INSERT INTO SceneNodeClass (pluginId,name,desc,classIdx) VALUES (?1,?2,?3,?4)";
static sqlite3_stmt* ADD_NODE_CLASS_INSERT_S;


/*
  * static const char ADD_NODE_CLASS_DELIN[] =
  * "DELETE FROM SceneNodeClassInput WHERE classId=?1";
  * static sqlite3_stmt* ADD_NODE_CLASS_DELIN_S;
  * static const char ADD_NODE_CLASS_DELOUT[] =
  * "DELETE FROM SceneNodeClassOutput WHERE classId=?1";
  * static sqlite3_stmt* ADD_NODE_CLASS_DELOUT_S;
  */
static const char ADD_NODE_CLASS_UPDIDX[] =
    "UPDATE SceneNodeClass SET arrayIdx=?2 WHERE id=?1";
static sqlite3_stmt* ADD_NODE_CLASS_UPDIDX_S;

int
lsddb_addNodeClass (struct LSD_SceneNodeClass** ptrToBind,
                    int pluginId,
                    const char* name,
                    const char* desc,
                    int classIdx)
{
    if (!name)
    {
        doLog (ERROR, LOG_COMP, _("Name not provided in addNodeClass()."));
        return -1;
    }
    if (!ptrToBind)
    {
        doLog (ERROR, LOG_COMP, _("Binding ptr not provided in addNodeClass(), unable to do anything useful."));
        return -1;
    }

    /* Check to see if class already registered and in array
     * if so */
    sqlite3_reset (ADD_NODE_CLASS_CHECK_S);

    sqlite3_bind_int (ADD_NODE_CLASS_CHECK_S, 1, pluginId);
    sqlite3_bind_text (ADD_NODE_CLASS_CHECK_S, 2, name, -1, NULL);

    int classId;
    if (sqlite3_step (ADD_NODE_CLASS_CHECK_S) == SQLITE_ROW)
    {
        int arrIdx = sqlite3_column_int (ADD_NODE_CLASS_CHECK_S, 0);
        classId = sqlite3_column_int (ADD_NODE_CLASS_CHECK_S, 1);
        if (arrIdx >= 0)  /* Already structed; pick and bind */
        {
            struct LSD_SceneNodeClass* classBind;
            if (pickIdx (getArr_lsdNodeClassArr (), (void**)&classBind,
                         arrIdx) < 0)
            {
                doLog (ERROR, LOG_COMP, _("Unable to resolve array entry %d in addNodeClass()."),
                         arrIdx);
                return -1;
            }
            if (classBind->dbId != classId)
            {
                doLog (ERROR, LOG_COMP, _("Picked class failed sanity check in addNodeClass()."));
                return -1;
            }
            *ptrToBind = classBind;
            return 0;
        }

        /* Struct record instead */
        doLog (NOTICE, LOG_COMP, _("Structing existing Class."));
    }
    else  /* NOT in DB; insert it and clear plugs */
    {
        sqlite3_reset (ADD_NODE_CLASS_INSERT_S);

        sqlite3_bind_int (ADD_NODE_CLASS_INSERT_S, 1, pluginId);
        sqlite3_bind_text (ADD_NODE_CLASS_INSERT_S, 2, name, -1, NULL);
        sqlite3_bind_text (ADD_NODE_CLASS_INSERT_S, 3, desc, -1, NULL);
        sqlite3_bind_int (ADD_NODE_CLASS_INSERT_S, 4, classIdx);

        if (sqlite3_step (ADD_NODE_CLASS_INSERT_S) != SQLITE_DONE)
        {
            doLog (ERROR, LOG_COMP, _("Unable to insert new class entry into DB in addNodeClass()\nDetails: %s"),
                   sqlite3_errmsg (memdb));
            return -1;
        }
        classId = sqlite3_last_insert_rowid (memdb);

    }

    /* Struct class here */
    size_t insertedIdx;
    struct LSD_SceneNodeClass* insertedClass;
    if (insertElem (getArr_lsdNodeClassArr (), &insertedIdx,
                    (void**)&insertedClass) < 0)
    {
        doLog (ERROR, LOG_COMP, _("Unable to insert class into array in addNodeClass()."));
        return -1;
    }

    insertedClass->dbId = classId;

    /* Update arrIdx */
    sqlite3_reset (ADD_NODE_CLASS_UPDIDX_S);

    sqlite3_bind_int (ADD_NODE_CLASS_UPDIDX_S, 1, classId);
    sqlite3_bind_int (ADD_NODE_CLASS_UPDIDX_S, 2, insertedIdx);

    if (sqlite3_step (ADD_NODE_CLASS_UPDIDX_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Unable to update arrIdx in addNodeClass()."));
        return -1;
    }

    *ptrToBind = insertedClass;
    return 0;
}


static const char ADD_DATA_TYPE_CHECK[] =
    "SELECT id FROM SceneDataType WHERE pluginId=?1 AND name=?2";
static sqlite3_stmt* ADD_DATA_TYPE_CHECK_S;

static const char ADD_DATA_TYPE_INSERT[] =
    "INSERT INTO SceneDataType (pluginId,name,desc) VALUES (?1,?2,?3)";
static sqlite3_stmt* ADD_DATA_TYPE_INSERT_S;

int
lsddb_addDataType (int* ptrToBind, int pluginId, const char* name,
                   const char* desc)
{
    if (!name)
    {
        doLog (ERROR, LOG_COMP, _("Name not provided in addDataType()."));
        return -1;
    }
    if (!ptrToBind)
    {
        doLog (ERROR, LOG_COMP, _("Binding ptr not provided in addDataType(), unable to do anything useful."));
        return -1;
    }

    /* Check to see if type already registered in DB and
     * array if so */
    sqlite3_reset (ADD_DATA_TYPE_CHECK_S);

    sqlite3_bind_int (ADD_DATA_TYPE_CHECK_S, 1, pluginId);
    sqlite3_bind_text (ADD_DATA_TYPE_CHECK_S, 2, name, -1, NULL);

    int typeId;
    if (sqlite3_step (ADD_DATA_TYPE_CHECK_S) == SQLITE_ROW)
        typeId = sqlite3_column_int (ADD_DATA_TYPE_CHECK_S, 0);
    else  /* Not in DB; insert it */
    {
        sqlite3_reset (ADD_DATA_TYPE_INSERT_S);

        sqlite3_bind_int (ADD_DATA_TYPE_INSERT_S, 1, pluginId);
        sqlite3_bind_text (ADD_DATA_TYPE_INSERT_S, 2, name, -1, NULL);
        sqlite3_bind_text (ADD_DATA_TYPE_INSERT_S, 3, desc, -1, NULL);

        if (sqlite3_step (ADD_DATA_TYPE_INSERT_S) != SQLITE_DONE)
        {
            doLog (ERROR, LOG_COMP, _("Unable to insert new class entry into DB in addDataType()\nDetails: %s"),
                   sqlite3_errmsg (memdb));
            return -1;
        }
        typeId = sqlite3_last_insert_rowid (memdb);
    }

    *ptrToBind = typeId;
    return 0;
}


/* OUTPUT Constructor: To be ran while node insts are being
 * inserted */
static const char STRUCT_NODE_INST_OUTPUT_ARR[] =
    "SELECT id,typeId,bfFuncIdx,bpFuncIdx FROM SceneNodeInstOutput WHERE "
    "instId=?1 AND facadeBool=0";
static sqlite3_stmt* STRUCT_NODE_INST_OUTPUT_ARR_S;

static const char STRUCT_NODE_INST_OUTPUT_ARR_UPDIDX[] =
    "UPDATE SceneNodeInstOutput SET arrIdx=?1 WHERE id=?2";
static sqlite3_stmt* STRUCT_NODE_INST_OUTPUT_ARR_UPDIDX_S;

int
lsddb_structNodeInstOutputArr (struct LSD_SceneNodeInst* nodeInst)
{
    if (!nodeInst)
    {
        doLog (ERROR, LOG_COMP, _("nodeInst may not be null in structNodeInstOutputArr()."));
        return -1;
    }

    sqlite3_reset (STRUCT_NODE_INST_OUTPUT_ARR_S);
    sqlite3_bind_int (STRUCT_NODE_INST_OUTPUT_ARR_S, 1, nodeInst->dbId);

    int errcode;
    while (( errcode = sqlite3_step (STRUCT_NODE_INST_OUTPUT_ARR_S)) ==
           SQLITE_ROW)
    {
        int outId = sqlite3_column_int (STRUCT_NODE_INST_OUTPUT_ARR_S, 0);
        int typeId = sqlite3_column_int (STRUCT_NODE_INST_OUTPUT_ARR_S, 1);
        int bfFuncIdx = sqlite3_column_int (STRUCT_NODE_INST_OUTPUT_ARR_S, 2);
        int bpFuncIdx = sqlite3_column_int (STRUCT_NODE_INST_OUTPUT_ARR_S, 3);

        size_t outArrIdx;
        struct LSD_SceneNodeOutput* nodeOut;
        if (insertElem (getArr_lsdNodeOutputArr (), &outArrIdx,
                        (void**)&nodeOut) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to insert inst output into array in structNodeInstOutputArr()."));
            return -1;
        }

        nodeOut->dbId = outId;
        nodeOut->typeId = typeId;
        nodeOut->parentNode = nodeInst;

        /* Ensure function pointers are back in place */
        nodeOut->bufferFunc = nodeInst->nodeClass->bfFuncTbl[bfFuncIdx];
        nodeOut->bufferPtr = nodeInst->nodeClass->bpFuncTbl[bpFuncIdx];
        /* printf("Set Node's output pointer func to
         * %d\n",bpFuncIdx); */
        /* nodeOut->bufferPtr(nodeOut); */

        /* Update Output ArrIdx */
        sqlite3_reset (STRUCT_NODE_INST_OUTPUT_ARR_UPDIDX_S);
        sqlite3_bind_int (STRUCT_NODE_INST_OUTPUT_ARR_UPDIDX_S, 1, outArrIdx);
        sqlite3_bind_int (STRUCT_NODE_INST_OUTPUT_ARR_UPDIDX_S, 2, outId);

        if (sqlite3_step (STRUCT_NODE_INST_OUTPUT_ARR_UPDIDX_S) != SQLITE_DONE)
        {
            doLog (ERROR, LOG_COMP, _("Unable to update node output's array index in structNodeInstOutputArr()."));
            return -1;
        }
    }
    if (errcode != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("structNodeInstOutputArr did not loop cleanly."));
        return -1;
    }
    return 0;
}


/* INPUT Constructor: To be ran while node insts are being
 * inserted */
static const char STRUCT_NODE_INST_INPUT_ARR[] =
    "SELECT id,typeId FROM SceneNodeInstInput WHERE "
    "instId=?1 AND facadeBool=0";
static sqlite3_stmt* STRUCT_NODE_INST_INPUT_ARR_S;

static const char STRUCT_NODE_INST_INPUT_ARR_UPDIDX[] =
    "UPDATE SceneNodeInstInput SET arrIdx=?1 WHERE id=?2";
static sqlite3_stmt* STRUCT_NODE_INST_INPUT_ARR_UPDIDX_S;

int
lsddb_structNodeInstInputArr (struct LSD_SceneNodeInst* nodeInst)
{
    if (!nodeInst)
    {
        doLog (ERROR, LOG_COMP, _("nodeInst may not be null in structNodeInstInputArr()."));
        return -1;
    }

    sqlite3_reset (STRUCT_NODE_INST_INPUT_ARR_S);
    sqlite3_bind_int (STRUCT_NODE_INST_INPUT_ARR_S, 1, nodeInst->dbId);

    int errcode;
    while (( errcode = sqlite3_step (STRUCT_NODE_INST_INPUT_ARR_S)) ==
           SQLITE_ROW)
    {
        int inId = sqlite3_column_int (STRUCT_NODE_INST_INPUT_ARR_S, 0);
        int typeId = sqlite3_column_int (STRUCT_NODE_INST_INPUT_ARR_S, 1);

        size_t inArrIdx;
        struct LSD_SceneNodeInput* nodeIn;
        if (insertElem (getArr_lsdNodeInputArr (), &inArrIdx,
                        (void**)&nodeIn) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to insert inst output in array in structNodeInstInputArr()."));
            return -1;
        }

        nodeIn->dbId = inId;
        nodeIn->typeId = typeId;
        nodeIn->parentNode = nodeInst;

        /* update Input arrIdx */
        sqlite3_reset (STRUCT_NODE_INST_INPUT_ARR_UPDIDX_S);
        sqlite3_bind_int (STRUCT_NODE_INST_INPUT_ARR_UPDIDX_S, 1, inArrIdx);
        sqlite3_bind_int (STRUCT_NODE_INST_INPUT_ARR_UPDIDX_S, 2, inId);

        if (sqlite3_step (STRUCT_NODE_INST_INPUT_ARR_UPDIDX_S) != SQLITE_DONE)
        {
            doLog (ERROR, LOG_COMP, _("Unable to update node input's array index in structNodeInstInputArr()."));
            return -1;
        }
    }
    if (errcode != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("structNodeInstInputArr did not loop cleanly."));
        return -1;
    }
    return 0;
}


/* For a given patchSpace, this function iteratively
 * constructs each */
/* node within the patchSpace. This function is only ran at
 * initialisation */
/* Any further node insertions at runtime are performed
 * automatically */
/* as part of the API function. */

/* A fully constructed node has memory allocated according
 * to the value in */
/* the class's instDataSize member. */

static const char STRUCT_NODE_INST_ARR_CHECK_ENABLE[] =
    "SELECT ScenePlugin.enabled,ScenePlugin.id FROM ScenePlugin,SceneNodeClass WHERE "
    "ScenePlugin.id=SceneNodeClass.pluginId AND SceneNodeClass.id=?1";
static sqlite3_stmt* STRUCT_NODE_INST_ARR_CHECK_ENABLE_S;

/* Get Node's plugin enable status */
int
lsddb_checkClassEnabled (int classId)
{
    int nodeEnabled = 0;
    int pluginId = -1;
    sqlite3_reset (STRUCT_NODE_INST_ARR_CHECK_ENABLE_S);
    sqlite3_bind_int (STRUCT_NODE_INST_ARR_CHECK_ENABLE_S, 1, classId);
    if (sqlite3_step (STRUCT_NODE_INST_ARR_CHECK_ENABLE_S) == SQLITE_ROW)
    {
        nodeEnabled = sqlite3_column_int (STRUCT_NODE_INST_ARR_CHECK_ENABLE_S,
                                          0);
        pluginId = sqlite3_column_int (STRUCT_NODE_INST_ARR_CHECK_ENABLE_S, 1);
    }

    /* Exception: all classes of core plugin (1) are enabled
     * by default */
    if (pluginId == 1)
        return 1;

    return nodeEnabled;
}


static const char CHECK_INPUT_ENABLE[] =
    "SELECT SceneNodeInst.classId FROM SceneNodeInst,SceneNodeInstInput WHERE "
    "SceneNodeInstInput.facadeBool=0 AND SceneNodeInstInput.instId=SceneNodeInst.id "
    "AND SceneNodeInstInput.id=?1";
static sqlite3_stmt* CHECK_INPUT_ENABLE_S;

int
lsddb_getInputClassId (int inId)
{
    sqlite3_reset (CHECK_INPUT_ENABLE_S);
    sqlite3_bind_int (CHECK_INPUT_ENABLE_S, 1, inId);
    if (sqlite3_step (CHECK_INPUT_ENABLE_S) == SQLITE_ROW)
        return sqlite3_column_int (CHECK_INPUT_ENABLE_S, 0);
    return -2;
}


static const char CHECK_OUTPUT_ENABLE[] =
    "SELECT SceneNodeInst.classId FROM SceneNodeInst,SceneNodeInstOutput WHERE "
    "SceneNodeInstOutput.facadeBool=0 AND SceneNodeInstOutput.instId=SceneNodeInst.id "
    "AND SceneNodeInstOutput.id=?1";
static sqlite3_stmt* CHECK_OUTPUT_ENABLE_S;

int
lsddb_getOutputClassId (int outId)
{
    sqlite3_reset (CHECK_OUTPUT_ENABLE_S);
    sqlite3_bind_int (CHECK_OUTPUT_ENABLE_S, 1, outId);
    if (sqlite3_step (CHECK_OUTPUT_ENABLE_S) == SQLITE_ROW)
        return sqlite3_column_int (CHECK_OUTPUT_ENABLE_S, 0);
    return -2;
}


static const char STRUCT_NODE_INST_ARR[] =
    "SELECT id,classId FROM SceneNodeInst WHERE patchSpaceId=?1";
static const char STRUCT_NODE_INST_ARR_UPDIDX[] =
    "UPDATE SceneNodeInst SET arrIdx=?1 WHERE id=?2";
static sqlite3_stmt* STRUCT_NODE_INST_ARR_S;
static sqlite3_stmt* STRUCT_NODE_INST_ARR_UPDIDX_S;

int
lsddb_structNodeInstArr (int patchSpaceId)
{
    sqlite3_reset (STRUCT_NODE_INST_ARR_S);
    sqlite3_bind_int (STRUCT_NODE_INST_ARR_S, 1, patchSpaceId);

    int errcode;
    while (( errcode = sqlite3_step (STRUCT_NODE_INST_ARR_S)) == SQLITE_ROW)
    {
        int instId = sqlite3_column_int (STRUCT_NODE_INST_ARR_S, 0);
        int classId = sqlite3_column_int (STRUCT_NODE_INST_ARR_S, 1);

        int nodeEnabled = lsddb_checkClassEnabled (classId);

        if (instId != 0 && nodeEnabled)
        {
            size_t targetIdx;
            struct LSD_SceneNodeInst* nodeInst;
            if (insertElem (getArr_lsdNodeInstArr (), &targetIdx,
                            (void**)&nodeInst) < 0)
            {
                doLog (ERROR, LOG_COMP, _("Unable to insert array element in structNodeInstArr()."));
                return -1;
            }
            /* printf("Structed Node Inst from patchSpace
             * %d\n",patchSpaceId); */

            nodeInst->dbId = instId;

            /* Reconnect node's class */
            if (lsddb_resolveClassFromId (&( nodeInst->nodeClass ),
                                          classId) < 0)
                doLog (ERROR, LOG_COMP, _("Unable to resolve node's class while restructing."));


            sqlite3_reset (STRUCT_NODE_INST_ARR_UPDIDX_S);
            sqlite3_bind_int (STRUCT_NODE_INST_ARR_UPDIDX_S, 1, targetIdx);
            sqlite3_bind_int (STRUCT_NODE_INST_ARR_UPDIDX_S, 2, instId);

            if (sqlite3_step (STRUCT_NODE_INST_ARR_UPDIDX_S) != SQLITE_DONE)
            {
                doLog (ERROR, LOG_COMP, _("Error while updating node inst arr idx in structNodeInstArr()."));
                return -1;
            }

            /* Struct this instance's inputs and outputs */
            if (lsddb_structNodeInstInputArr (nodeInst) < 0)
            {
                doLog (ERROR, LOG_COMP, _("Unable to struct node's inputs in structNodeInstArr()."));
                return -1;
            }
            if (lsddb_structNodeInstOutputArr (nodeInst) < 0)
            {
                doLog (ERROR, LOG_COMP, _("Unable to struct node's outputs in structNodeInstArr()."));
                return -1;
            }

            /* Allocate inst's memory */
            if (nodeInst->nodeClass->instDataSize > 0)
            {
                nodeInst->data = malloc (nodeInst->nodeClass->instDataSize);

                if (!nodeInst->data)
                {
                    doLog (ERROR, LOG_COMP, _("Unable to allocate memory for node inst data in structNodeInstArr()."));
                    return -1;
                }
            }

            /* Run restore func */
            if (nodeInst->nodeClass->nodeRestoreFunc)
                nodeInst->nodeClass->nodeRestoreFunc (nodeInst, nodeInst->data);
        }
    }
    if (errcode != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("structNodeInstArr() did not loop cleanly."));
        return -1;
    }
    return 0;
}


/* Private function for constructing universes on behalf of
 * partitions constructor */
static const char STRUCT_UNIV_ARR[] =
    "SELECT DISTINCT olaUnivId FROM OlaAddress";
static sqlite3_stmt* STRUCT_UNIV_ARR_S;

static const char STRUCT_UNIV_ARR_UPDIDX[] =
    "UPDATE OlaAddress SET olaUnivArrIdx=?1 WHERE olaUnivId=?2";
static sqlite3_stmt* STRUCT_UNIV_ARR_UPDIDX_S;

static const char STRUCT_UNIV_ARR_MAXIDX[] =
    "SELECT olaLightAddr FROM OlaAddress WHERE olaUnivId=?1 ORDER BY olaLightAddr DESC LIMIT 1";
static sqlite3_stmt* STRUCT_UNIV_ARR_MAXIDX_S;

int
lsddb_structUnivArr ()
{
    sqlite3_reset (STRUCT_UNIV_ARR_S);
    int errcode;
    while (( errcode = sqlite3_step (STRUCT_UNIV_ARR_S)) == SQLITE_ROW)
    {
        int univId;
        univId = sqlite3_column_int (STRUCT_UNIV_ARR_S, 0);

        size_t univArrIdx;
        struct LSD_Univ* univPtr;
        if (insertElem (getArr_lsdUnivArr (), &univArrIdx,
                        (void**)&univPtr) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to allocate array space in structUnivArr()."));
            return -1;
        }

        sqlite3_reset (STRUCT_UNIV_ARR_UPDIDX_S);
        sqlite3_bind_int (STRUCT_UNIV_ARR_UPDIDX_S, 1, univArrIdx);
        sqlite3_bind_int (STRUCT_UNIV_ARR_UPDIDX_S, 2, univId);

        if (sqlite3_step (STRUCT_UNIV_ARR_UPDIDX_S) != SQLITE_DONE)
        {
            doLog (ERROR, LOG_COMP, _("structUnivArr() was unable to update an instance's arrayIdx."));
            return -1;
        }

        sqlite3_reset (STRUCT_UNIV_ARR_MAXIDX_S);
        sqlite3_bind_int (STRUCT_UNIV_ARR_MAXIDX_S, 1, univId);

        int maxIdx;
        if (sqlite3_step (STRUCT_UNIV_ARR_MAXIDX_S) == SQLITE_ROW)
            maxIdx = sqlite3_column_int (STRUCT_UNIV_ARR_MAXIDX_S, 0);
        else
            maxIdx = -1;


        univPtr->olaUnivId = univId;
        univPtr->maxIdx = maxIdx;
        if (maxIdx >= 0)
        {
            uint8_t* univBuf = malloc (sizeof( uint8_t ) * ( maxIdx + 2 ));
            if (univBuf)
                univPtr->buffer = univBuf;
            else
            {
                doLog (ERROR, LOG_COMP, _("Unable to allocate memory for DMX buffer."));
                return -1;
            }
        }
    }
    if (errcode != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("structUnivArr() did not loop cleanly."));
        return -1;
    }
    return 0;
}


/* Private function for constructing address on behalf of
 * channels constructor */
static const char STRUCT_CHANNEL_ARR_ADDR[] =
    "SELECT id,olaUnivArrIdx,olaLightAddr,sixteenBit FROM OlaAddress WHERE id=?1";
static sqlite3_stmt* STRUCT_CHANNEL_ARR_ADDR_S;

int
lsddb_structChannelArrAddr (struct LSD_Addr* addr, int addrId)
{
    if (!addr)
    {
        doLog (ERROR, LOG_COMP, _("addr is NULL in structChannelArrAddr()."));
        return -1;
    }

    sqlite3_reset (STRUCT_CHANNEL_ARR_ADDR_S);
    sqlite3_bind_int (STRUCT_CHANNEL_ARR_ADDR_S, 1, addrId);

    if (sqlite3_step (STRUCT_CHANNEL_ARR_ADDR_S) == SQLITE_ROW)
    {
        int addrId = sqlite3_column_int (STRUCT_CHANNEL_ARR_ADDR_S, 0);
        int univIdx = sqlite3_column_int (STRUCT_CHANNEL_ARR_ADDR_S, 1);
        int lightAddr = sqlite3_column_int (STRUCT_CHANNEL_ARR_ADDR_S, 2);
        int b16 = sqlite3_column_int (STRUCT_CHANNEL_ARR_ADDR_S, 3);

        struct LSD_Univ* univPtr;
        if (pickIdx (getArr_lsdUnivArr (), (void**)&univPtr, univIdx) < 0)
        {
            doLog (ERROR, LOG_COMP, _("There was a problem while picking universe in structChannelArr()."));
            return -1;
        }

        if (lightAddr > univPtr->maxIdx)
        {
            doLog (ERROR, LOG_COMP, _("Light address is outside bounds of its universe structure."));
            return -1;
        }

        addr->dbId = addrId;
        addr->univ = univPtr;
        addr->addr = lightAddr;
        addr->b16 = b16;

        return 0;
    }
    doLog (ERROR, LOG_COMP, _("Unable to find DB record for requested address in structChannelArrAddr."));
    return -1;
}


/* Private function for constructing channels on bahalf of
 * partitions constructor */
static const char STRUCT_CHANNEL_ARR[] =
    "SELECT id,single,rAddrId,gAddrId,bAddrId,facadeOutId FROM SystemChannel";
static sqlite3_stmt* STRUCT_CHANNEL_ARR_S;

static const char STRUCT_CHANNEL_ARR_UPDIDX[] =
    "UPDATE SystemChannel SET arrIdx=?2 WHERE id=?1";
static sqlite3_stmt* STRUCT_CHANNEL_ARR_UPDIDX_S;

int
lsddb_structChannelArr ()
{
    sqlite3_reset (STRUCT_CHANNEL_ARR_S);
    int errcode;
    while (( errcode = sqlite3_step (STRUCT_CHANNEL_ARR_S)) == SQLITE_ROW)
    {
        int chanId = sqlite3_column_int (STRUCT_CHANNEL_ARR_S, 0);
        int chanSingle = sqlite3_column_int (STRUCT_CHANNEL_ARR_S, 1);
        int chanRa = sqlite3_column_int (STRUCT_CHANNEL_ARR_S, 2);
        int chanGa = sqlite3_column_int (STRUCT_CHANNEL_ARR_S, 3);
        int chanBa = sqlite3_column_int (STRUCT_CHANNEL_ARR_S, 4);
        int facadeOutId = sqlite3_column_int (STRUCT_CHANNEL_ARR_S, 5);

        size_t channelIdx;
        struct LSD_Channel* chanBind;
        if (insertElem (getArr_lsdChannelArr (), &channelIdx,
                        (void**)&chanBind) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to insert channel into array in structChannelArr()."));
            return -1;
        }

        chanBind->dbId = chanId;

        if (chanSingle)
        {
            chanBind->single = 1;
            if (lsddb_structChannelArrAddr (&( chanBind->rAddr ), chanRa) < 0)
            {
                doLog (ERROR, LOG_COMP, _("Unable to struct single addr in structChannelArr()."));
                return -1;
            }
        }
        else
        {
            chanBind->single = 0;
            if (lsddb_structChannelArrAddr (&( chanBind->rAddr ), chanRa) < 0)
            {
                doLog (ERROR, LOG_COMP, _("Unable to struct red addr in structChannelArr()."));
                return -1;
            }
            if (lsddb_structChannelArrAddr (&( chanBind->gAddr ), chanGa) < 0)
            {
                doLog (ERROR, LOG_COMP, _("Unable to struct green addr in structChannelArr()."));
                return -1;
            }
            if (lsddb_structChannelArrAddr (&( chanBind->bAddr ), chanBa) < 0)
            {
                doLog (ERROR, LOG_COMP, _("Unable to struct blue addr in structChannelArr()."));
                return -1;
            }
        }

        /* Resolve and set channel object's aliased output
         * (if exists) */
        chanBind->output = NULL;
        lsddb_traceOutput (&( chanBind->output ), facadeOutId, NULL, NULL);
        if (chanBind->output)
            doLog (NOTICE, LOG_COMP, _("structChannelArr() output id %d."), chanBind->output->dbId);

        /* Update Channel's ArrIdx */
        sqlite3_reset (STRUCT_CHANNEL_ARR_UPDIDX_S);
        sqlite3_bind_int (STRUCT_CHANNEL_ARR_UPDIDX_S, 1, chanId);
        sqlite3_bind_int (STRUCT_CHANNEL_ARR_UPDIDX_S, 2, channelIdx);
        if (sqlite3_step (STRUCT_CHANNEL_ARR_UPDIDX_S) != SQLITE_DONE)
        {
            doLog (ERROR, LOG_COMP, _("Unable to update channel's arrIdx in structChannelArr()."));
            return -1;
        }

    }
    if (errcode != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("structChannelArr() did not loop cleanly."));
        return -1;
    }
    return 0;
}


/* Only to be called at LSD init after memdb restore is
 * complete */
/* Iteratively does the following for each partition: */
/*      - Completes the partition's patch space set
 * according to db */
/*          - Nodes registered in array (and their required
 * structure buffers made available) (structNodeArr) */
/*          - Nodes' plugs registered in array (output
 * buffers made available) */
/*              - Class ins and outs registered with the
 * node's class are instantiated here (described below) */
/*              - Node's ins and outs are registered in
 * array while ensuring output buffers constitute */
/*                  valid pointers to a buffer sized by
 * SceneNodeClass's nodeInstSize */
/*              - Node records (and IO records) are updated
 * in the DB with new array indicies */
/*      - The partition's facade pointer is picked from the
 * node array (using DB's now registered node index) */
/*      - The partition's facade node structure harbours one
 * SceneNodeInput of type RGB[Array] (also picked for
 * operation) */

/* Should be ran during init after [plugins (nodeClasses and
 * types in turn), universes, and channels] have been
 * structed (in that order) */
static const char STRUCT_PARTITION_ARR[] =
    "SELECT id,patchSpaceId FROM SystemPartition";
static sqlite3_stmt* STRUCT_PARTITION_ARR_S;

static const char STRUCT_PARTITION_ARR_UPDIDX[] =
    "UPDATE SystemPartition SET arrayIdx=?1 WHERE id=?2";
static sqlite3_stmt* STRUCT_PARTITION_ARR_UPDIDX_S;

int
lsddb_structPartitionArr ()
{

    /* First insert partition facade nodes */
    if (lsddb_structNodeInstArr (0) < 0)
    {
        doLog (ERROR, LOG_COMP, _("Unable to struct partition facade nodes in structPartitionArr()."));
        return -1;
    }

    sqlite3_reset (STRUCT_PARTITION_ARR_S);

    int errcode;
    while (( errcode = sqlite3_step (STRUCT_PARTITION_ARR_S)) == SQLITE_ROW)
    {
        /* Collect partition's data from DB */
        int partId = sqlite3_column_int (STRUCT_PARTITION_ARR_S, 0);
        int patchSpaceId = sqlite3_column_int (STRUCT_PARTITION_ARR_S, 1);

        /* Insert corresponding partition structure */
        size_t partArrIdx;
        struct LSD_Partition* partArrPtr;
        if (insertElem (getArr_lsdPartitionArr (), &partArrIdx,
                        (void**)&partArrPtr) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to insert Partition into array in createPartition()."));
            return -1;
        }

        /* Construct nodes in the partition's contained
         * patchSpace */
        if (lsddb_structNodeInstArr (patchSpaceId) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to insert nodes contained within partition's patch space in structPartitionArr()."));
            return -1;
        }

        /* Construct nodes within facades in the partition's
         * patchSpace */
        sqlite3_reset (REMOVE_PATCH_SPACE_FACADES_S);
        sqlite3_bind_int (REMOVE_PATCH_SPACE_FACADES_S, 1, patchSpaceId);
        while (sqlite3_step (REMOVE_PATCH_SPACE_FACADES_S) == SQLITE_ROW)
        {
            int cpsId = sqlite3_column_int (REMOVE_PATCH_SPACE_FACADES_S, 0);
            lsddb_structNodeInstArr (cpsId);
        }

        /* Update arr idx */
        sqlite3_reset (STRUCT_PARTITION_ARR_UPDIDX_S);
        sqlite3_bind_int (STRUCT_PARTITION_ARR_UPDIDX_S, 1, partArrIdx);
        sqlite3_bind_int (STRUCT_PARTITION_ARR_UPDIDX_S, 2, partId);

        if (sqlite3_step (STRUCT_PARTITION_ARR_UPDIDX_S) != SQLITE_DONE)
        {
            doLog (ERROR, LOG_COMP, _("There was a problem updating the partition "
                     "arr idx in structPartitionArr()."));
            return -1;
        }

        /* Make appropriate links */
        partArrPtr->dbId = partId;
        /* partArrPtr->facade = partFacade; */
    }
    if (errcode != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("structPartitionArr() did not loop cleanly."));
        return -1;
    }

    /* Iterate to reestablish wire connections */
    lsddb_rewireNodes ();

    return 0;
}


static const char ADD_NODE_INST_INPUT_INSERT[] =
    "INSERT INTO SceneNodeInstInput (instId,typeId,name,facadeBool) VALUES (?1,?2,?3,0)";
static sqlite3_stmt* ADD_NODE_INST_INPUT_INSERT_S;

static const char ADD_NODE_INST_INPUT_UPDIDX[] =
    "UPDATE SceneNodeInstInput SET arrIdx=?2 WHERE id=?1";
static sqlite3_stmt* ADD_NODE_INST_INPUT_UPDIDX_S;

int
lsddb_addNodeInstInput (struct LSD_SceneNodeInst const* node,
                        int typeId,
                        const char* name,
                        struct LSD_SceneNodeInput** ptrToBind,
                        int* idBinding)
{
    if (!name)
    {
        doLog (ERROR, LOG_COMP, _("Improper arguments passed to addNodeInstInput()."));
        return -1;
    }

    sqlite3_reset (ADD_NODE_INST_INPUT_INSERT_S);
    sqlite3_bind_int (ADD_NODE_INST_INPUT_INSERT_S, 1, node->dbId);
    sqlite3_bind_int (ADD_NODE_INST_INPUT_INSERT_S, 2, typeId);
    sqlite3_bind_text (ADD_NODE_INST_INPUT_INSERT_S, 3, name, -1, NULL);

    if (sqlite3_step (ADD_NODE_INST_INPUT_INSERT_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Error while inserting input into DB."));
        return -1;
    }

    int inputId = sqlite3_last_insert_rowid (memdb);

    /* Insert into array */
    size_t arrIdx;
    struct LSD_SceneNodeInput* input;
    if (insertElem (getArr_lsdNodeInputArr (), &arrIdx, (void**)&input) < 0)
    {
        doLog (ERROR, LOG_COMP, _("Unable to insert input into array in addNodeInstInput()."));
        return -1;
    }

    /* Set arrIdx in DB */
    sqlite3_reset (ADD_NODE_INST_INPUT_UPDIDX_S);
    sqlite3_bind_int (ADD_NODE_INST_INPUT_UPDIDX_S, 1, inputId);
    sqlite3_bind_int (ADD_NODE_INST_INPUT_UPDIDX_S, 2, arrIdx);

    if (sqlite3_step (ADD_NODE_INST_INPUT_UPDIDX_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Unable to update arrIdx in addNodeInstInput()."));
        return -1;
    }

    /* Populate input object */
    input->dbId = inputId;
    input->typeId = typeId;
    input->parentNode = node;
    input->connection = NULL;

    if (ptrToBind)
        *ptrToBind = input;
    if (idBinding)
        *idBinding = inputId;

    return 0;

}


static const char ADD_NODE_INST_OUTPUT_INSERT[] =
    "INSERT INTO SceneNodeInstOutput (instId,typeId,name,facadeBool,bfFuncIdx,bpFuncIdx) VALUES (?1,?2,?3,0,?4,?5)";
static sqlite3_stmt* ADD_NODE_INST_OUTPUT_INSERT_S;

static const char ADD_NODE_INST_OUTPUT_UPDIDX[] =
    "UPDATE SceneNodeInstOutput SET arrIdx=?2 WHERE id=?1";
static sqlite3_stmt* ADD_NODE_INST_OUTPUT_UPDIDX_S;

int
lsddb_addNodeInstOutput (struct LSD_SceneNodeInst const* node,
                         int typeId,
                         const char* name,
                         struct LSD_SceneNodeOutput** ptrToBind,
                         int* idBinding,
                         int bfFuncIdx,
                         int bpFuncIdx)
{
    if (!name)
    {
        doLog (ERROR, LOG_COMP, _("Improper arguments passed to addNodeInstOutput()."));
        return -1;
    }

    sqlite3_reset (ADD_NODE_INST_OUTPUT_INSERT_S);
    sqlite3_bind_int (ADD_NODE_INST_OUTPUT_INSERT_S, 1, node->dbId);
    sqlite3_bind_int (ADD_NODE_INST_OUTPUT_INSERT_S, 2, typeId);
    sqlite3_bind_text (ADD_NODE_INST_OUTPUT_INSERT_S, 3, name, -1, NULL);
    sqlite3_bind_int (ADD_NODE_INST_OUTPUT_INSERT_S, 4, bfFuncIdx);
    sqlite3_bind_int (ADD_NODE_INST_OUTPUT_INSERT_S, 5, bpFuncIdx);

    if (sqlite3_step (ADD_NODE_INST_OUTPUT_INSERT_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Error while inserting output into DB."));
        return -1;
    }

    int outputId = sqlite3_last_insert_rowid (memdb);

    /* Insert into array */
    size_t arrIdx;
    struct LSD_SceneNodeOutput* output;
    if (insertElem (getArr_lsdNodeOutputArr (), &arrIdx, (void**)&output) < 0)
    {
        doLog (ERROR, LOG_COMP, _("Unable to insert output into array in addNodeInstOutput()."));
        return -1;
    }

    /* Set arrIdx in DB */
    sqlite3_reset (ADD_NODE_INST_OUTPUT_UPDIDX_S);
    sqlite3_bind_int (ADD_NODE_INST_OUTPUT_UPDIDX_S, 1, outputId);
    sqlite3_bind_int (ADD_NODE_INST_OUTPUT_UPDIDX_S, 2, arrIdx);

    if (sqlite3_step (ADD_NODE_INST_OUTPUT_UPDIDX_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Unable to update arrIdx in addNodeInstOutput()."));
        return -1;
    }

    /* Populate output object */
    output->dbId = outputId;
    output->typeId = typeId;
    output->parentNode = node;

    if (ptrToBind)
        *ptrToBind = output;
    if (idBinding)
        *idBinding = outputId;

    return 0;
}


static const char REMOVE_NODE_INST_INPUT_ARRIDX[] =
    "SELECT arrIdx FROM SceneNodeInstInput WHERE id=?1";
static sqlite3_stmt* REMOVE_NODE_INST_INPUT_ARRIDX_S;

static const char REMOVE_NODE_INST_INPUT[] =
    "DELETE FROM SceneNodeInstInput WHERE id=?1 AND facadeBool=0";
static sqlite3_stmt* REMOVE_NODE_INST_INPUT_S;

static const char REMOVE_NODE_INST_INPUT_GET_WIRES[] =
    "SELECT id FROM SceneNodeEdge WHERE destIn=?1 AND destFacadeInt=0";
static sqlite3_stmt* REMOVE_NODE_INST_INPUT_GET_WIRES_S;

int
lsddb_removeNodeInstInput (int inputId)
{
    /* call unwireNodes on wire connected to input */
    sqlite3_reset (REMOVE_NODE_INST_INPUT_GET_WIRES_S);
    sqlite3_bind_int (REMOVE_NODE_INST_INPUT_GET_WIRES_S, 1, inputId);

    while (sqlite3_step (REMOVE_NODE_INST_INPUT_GET_WIRES_S) == SQLITE_ROW)
    {
        int wireId = sqlite3_column_int (REMOVE_NODE_INST_INPUT_GET_WIRES_S, 0);
        lsddb_unwireNodes (wireId);
    }

    /* Remove input from array */
    sqlite3_reset (REMOVE_NODE_INST_INPUT_ARRIDX_S);
    sqlite3_bind_int (REMOVE_NODE_INST_INPUT_ARRIDX_S, 1, inputId);

    int arrIdx;
    if (sqlite3_step (REMOVE_NODE_INST_INPUT_ARRIDX_S) == SQLITE_ROW)
        arrIdx = sqlite3_column_int (REMOVE_NODE_INST_INPUT_ARRIDX_S, 0);
    else
    {
        doLog (ERROR, LOG_COMP, _("Unable to resolve array index in removeNodeInstInput()."));
        return -1;
    }

    if (arrIdx >= 0)
        if (delIdx (getArr_lsdNodeInputArr (), arrIdx) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to remove input from array in removeNodeInstInput()."));
            return -1;
        }

    /* remove record from DB */
    sqlite3_reset (REMOVE_NODE_INST_INPUT_S);
    sqlite3_bind_int (REMOVE_NODE_INST_INPUT_S, 1, inputId);

    if (sqlite3_step (REMOVE_NODE_INST_INPUT_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Unable to remove input from DB."));
        return -1;
    }

    return 0;
}


static const char REMOVE_NODE_INST_OUTPUT_ARRIDX[] =
    "SELECT arrIdx FROM SceneNodeInstOutput WHERE id=?1";
static sqlite3_stmt* REMOVE_NODE_INST_OUTPUT_ARRIDX_S;

static const char REMOVE_NODE_INST_OUTPUT[] =
    "DELETE FROM SceneNodeInstOutput WHERE id=?1 AND facadeBool=0";
static sqlite3_stmt* REMOVE_NODE_INST_OUTPUT_S;

static const char REMOVE_NODE_INST_OUTPUT_GET_WIRES[] =
    "SELECT id FROM SceneNodeEdge WHERE srcOut=?1";
static sqlite3_stmt* REMOVE_NODE_INST_OUTPUT_GET_WIRES_S;

int
lsddb_removeNodeInstOutput (int outputId)
{
    /* call unwireNodes on wire connected to output */
    sqlite3_reset (REMOVE_NODE_INST_OUTPUT_GET_WIRES_S);
    sqlite3_bind_int (REMOVE_NODE_INST_OUTPUT_GET_WIRES_S, 1, outputId);

    while (sqlite3_step (REMOVE_NODE_INST_OUTPUT_GET_WIRES_S) == SQLITE_ROW)
    {
        int wireId = sqlite3_column_int (REMOVE_NODE_INST_OUTPUT_GET_WIRES_S, 0);
        lsddb_unwireNodes (wireId);
    }

    /* Remove output from array */
    sqlite3_reset (REMOVE_NODE_INST_OUTPUT_ARRIDX_S);
    sqlite3_bind_int (REMOVE_NODE_INST_OUTPUT_ARRIDX_S, 1, outputId);

    int arrIdx;
    if (sqlite3_step (REMOVE_NODE_INST_OUTPUT_ARRIDX_S) == SQLITE_ROW)
        arrIdx = sqlite3_column_int (REMOVE_NODE_INST_OUTPUT_ARRIDX_S, 0);
    else
    {
        doLog (ERROR, LOG_COMP, _("Unable to resolve array index in removeNodeInstOutput()."));
        return -1;
    }

    if (arrIdx >= 0)
        if (delIdx (getArr_lsdNodeOutputArr (), arrIdx) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to remove output from array in removeNodeInstOutput()."));
            return -1;
        }

    /* remove record from DB */
    sqlite3_reset (REMOVE_NODE_INST_OUTPUT_S);
    sqlite3_bind_int (REMOVE_NODE_INST_OUTPUT_S, 1, outputId);

    if (sqlite3_step (REMOVE_NODE_INST_OUTPUT_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Unable to remove output from DB."));
        return -1;
    }

    return 0;
}


/**
  * Adds DB Records AND necessary data structures to the
  *state for dynamic insertion of nodes
  */
static const char ADD_NODE_INST[] =
    "INSERT INTO SceneNodeInst (arrIdx,classId,patchSpaceId,name) "
    "VALUES (?1,?2,?3,?4)";
static sqlite3_stmt* ADD_NODE_INST_S;

static const char GET_NODE_CLASS_NAME[] =
    "SELECT name FROM SceneNodeClass WHERE id=?1";
static sqlite3_stmt* GET_NODE_CLASS_NAME_S;

int
lsddb_addNodeInst (int patchSpaceId, struct LSD_SceneNodeClass* nc,
                   int* idBinding, struct LSD_SceneNodeInst** ptrToBind)
{
    if (!nc || nc->dbId == 0)
    {
        doLog (ERROR, LOG_COMP, _("Invalid NodeClass used in addNodeInst()."));
        if (!nc)
            doLog (ERROR, LOG_COMP, _("NULL NodeClass\n"));
        return -1;
    }

    sqlite3_reset (ADD_NODE_INST_S);

    size_t targetIdx;
    struct LSD_SceneNodeInst* targetPtr;
    if (insertElem (getArr_lsdNodeInstArr (), &targetIdx,
                    (void**)&targetPtr) < 0)
    {
        doLog (ERROR, LOG_COMP, _("Unable to insert node instance into array in addNodeInst()."));
        return -1;
    }

    sqlite3_bind_int (ADD_NODE_INST_S, 1, targetIdx);
    sqlite3_bind_int (ADD_NODE_INST_S, 2, nc->dbId);
    sqlite3_bind_int (ADD_NODE_INST_S, 3, patchSpaceId);

    /* Get class name to make default inst name */
    sqlite3_reset (GET_NODE_CLASS_NAME_S);
    sqlite3_bind_int (GET_NODE_CLASS_NAME_S, 1, nc->dbId);
    if (sqlite3_step (GET_NODE_CLASS_NAME_S) == SQLITE_ROW)
    {
        char newName[256];
        const unsigned char* claName = sqlite3_column_text (
            GET_NODE_CLASS_NAME_S,
            0);
        snprintf (newName, 256, "New %s", (const char*)claName);

        sqlite3_bind_text (ADD_NODE_INST_S, 4, newName, -1, NULL);
    }

    if (sqlite3_step (ADD_NODE_INST_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Error inserting element into DB in addNodeInst()\nDetails: %s"),
               sqlite3_errmsg (memdb));
        return -1;
    }

    int rowid = sqlite3_last_insert_rowid (memdb);

    if (idBinding)
        *idBinding = rowid;

    targetPtr->dbId = rowid;
    targetPtr->nodeClass = nc;
    if (nc->instDataSize > 0)
    {
        targetPtr->data = malloc (nc->instDataSize);

        if (!targetPtr->data)
        {
            doLog (ERROR, LOG_COMP, _("Unable to allocate memory for node inst data in addNodeInst()."));
            return -1;
        }
    }

    if (ptrToBind)
        *ptrToBind = targetPtr;

    if (nc->nodeMakeFunc)
        if (nc->nodeMakeFunc (targetPtr, targetPtr->data) < 0)
            doLog (ERROR, LOG_COMP, _("Plugin's Make function returned failure."));

    if (nc->nodeRestoreFunc)
        if (nc->nodeRestoreFunc (targetPtr, targetPtr->data) < 0)
            doLog (ERROR, LOG_COMP, _("Plugin's Restore function returned failure."));

    return 0;
}


static const char REMOVE_NODE_INST_CHECK[] =
    "SELECT arrIdx FROM SceneNodeInst WHERE id=?1";
static sqlite3_stmt* REMOVE_NODE_INST_CHECK_S;

static const char REMOVE_NODE_INST_GET_INS[] =
    "SELECT id FROM SceneNodeInstInput WHERE instId=?1 AND facadeBool=0";
static sqlite3_stmt* REMOVE_NODE_INST_GET_INS_S;

static const char REMOVE_NODE_INST_GET_OUTS[] =
    "SELECT id FROM SceneNodeInstOutput WHERE instId=?1 AND facadeBool=0";
static sqlite3_stmt* REMOVE_NODE_INST_GET_OUTS_S;

static const char REMOVE_NODE_INST_DELETE[] =
    "DELETE FROM SceneNodeInst WHERE id=?1";
static sqlite3_stmt* REMOVE_NODE_INST_DELETE_S;

int
lsddb_removeNodeInst (int nodeId)
{
    /* First check to see if the node exists, and get */
    /* its array index */
    sqlite3_reset (REMOVE_NODE_INST_CHECK_S);
    sqlite3_bind_int (REMOVE_NODE_INST_CHECK_S, 1, nodeId);

    if (sqlite3_step (REMOVE_NODE_INST_CHECK_S) == SQLITE_ROW)
    {
        int arrIdx = sqlite3_column_int (REMOVE_NODE_INST_CHECK_S, 0);

        /* Pick node, run delete, remove from array (which
         * runs clean) */
        struct LSD_SceneNodeInst* condemnedNode;

        if (pickIdx (getArr_lsdNodeInstArr (), (void**)&condemnedNode,
                     arrIdx) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to pick inst from array in removeNodeInst()."));
            return -1;
        }

        if (condemnedNode->nodeClass->nodeDeleteFunc)
            condemnedNode->nodeClass->nodeDeleteFunc (condemnedNode,
                                                      condemnedNode->data);

        if (delIdx (getArr_lsdNodeInstArr (), arrIdx) < 0)
            doLog (ERROR, LOG_COMP, _("Unable to remove node from array in removeNodeInst()."));

        /* Remove each inst input */
        sqlite3_reset (REMOVE_NODE_INST_GET_INS_S);
        sqlite3_bind_int (REMOVE_NODE_INST_GET_INS_S, 1, nodeId);

        while (sqlite3_step (REMOVE_NODE_INST_GET_INS_S) == SQLITE_ROW)
        {
            int inId = sqlite3_column_int (REMOVE_NODE_INST_GET_INS_S, 0);
            lsddb_removeNodeInstInput (inId);
        }

        /* Remove each inst output */
        sqlite3_reset (REMOVE_NODE_INST_GET_OUTS_S);
        sqlite3_bind_int (REMOVE_NODE_INST_GET_OUTS_S, 1, nodeId);

        while (sqlite3_step (REMOVE_NODE_INST_GET_OUTS_S) == SQLITE_ROW)
        {
            int outId = sqlite3_column_int (REMOVE_NODE_INST_GET_OUTS_S, 0);
            lsddb_removeNodeInstOutput (outId);
        }

        /* Remove node */
        sqlite3_reset (REMOVE_NODE_INST_DELETE_S);
        sqlite3_bind_int (REMOVE_NODE_INST_DELETE_S, 1, nodeId);
        if (sqlite3_step (REMOVE_NODE_INST_DELETE_S) != SQLITE_DONE)
        {
            doLog (ERROR, LOG_COMP, _("Unable to remove node from DB."));
            return -1;
        }

    }
    else
    {
        doLog (ERROR, LOG_COMP, _("Node inst non-existent in removeNodeInst()."));
        return -1;
    }

    return 0;
}


static const char UPDATE_NODE_NAME[] =
    "UPDATE SceneNodeInst SET name=?2 WHERE id=?1";
static sqlite3_stmt* UPDATE_NODE_NAME_S;

int
lsddb_updateNodeName (int nodeId, const char* name)
{
    if (!name)
        return -1;

    sqlite3_reset (UPDATE_NODE_NAME_S);
    sqlite3_bind_int (UPDATE_NODE_NAME_S, 1, nodeId);
    sqlite3_bind_text (UPDATE_NODE_NAME_S, 2, name, -1, NULL);

    if (sqlite3_step (UPDATE_NODE_NAME_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("error while updating node instance name."));
        return -1;
    }

    return 0;
}


/* Update node colour */
static const char SET_NODE_COLOUR[] =
    "UPDATE SceneNodeInst SET colourR=?2,colourG=?3,colourB=?4 WHERE id=?1";
static sqlite3_stmt* SET_NODE_COLOUR_S;

int
lsddb_setNodeColour (int nodeId, cJSON* colour)
{
    if (!colour || colour->type != cJSON_Object)
        return -1;

    cJSON* rObj = cJSON_GetObjectItem (colour, "r");
    cJSON* gObj = cJSON_GetObjectItem (colour, "g");
    cJSON* bObj = cJSON_GetObjectItem (colour, "b");

    if (!rObj || rObj->type != cJSON_Number ||
        !gObj || gObj->type != cJSON_Number ||
        !bObj || bObj->type != cJSON_Number)
        return -1;

    sqlite3_reset (SET_NODE_COLOUR_S);
    sqlite3_bind_int (SET_NODE_COLOUR_S, 1, nodeId);
    sqlite3_bind_double (SET_NODE_COLOUR_S, 2, rObj->valuedouble);
    sqlite3_bind_double (SET_NODE_COLOUR_S, 3, gObj->valuedouble);
    sqlite3_bind_double (SET_NODE_COLOUR_S, 4, bObj->valuedouble);

    if (sqlite3_step (SET_NODE_COLOUR_S) != SQLITE_DONE)
        return -1;
    return 0;
}


static const char NODE_INST_POS[] =
    "UPDATE SceneNodeInst SET posX=?2,posY=?3 WHERE id=?1";
static sqlite3_stmt* NODE_INST_POS_S;

int
lsddb_nodeInstPos (int nodeId, int xVal, int yVal)
{
    sqlite3_reset (NODE_INST_POS_S);
    sqlite3_bind_int (NODE_INST_POS_S, 1, nodeId);
    sqlite3_bind_int (NODE_INST_POS_S, 2, xVal);
    sqlite3_bind_int (NODE_INST_POS_S, 3, yVal);

    if (sqlite3_step (NODE_INST_POS_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Error while updating node position values."));
        return -1;
    }

    return 0;
}


static const char FACADE_INST_POS[] =
    "UPDATE ScenePatchSpace SET posX=?2,posY=?3 WHERE id=?1";
static sqlite3_stmt* FACADE_INST_POS_S;

int
lsddb_facadeInstPos (int nodeId, int xVal, int yVal)
{
    sqlite3_reset (FACADE_INST_POS_S);
    sqlite3_bind_int (FACADE_INST_POS_S, 1, nodeId);
    sqlite3_bind_int (FACADE_INST_POS_S, 2, xVal);
    sqlite3_bind_int (FACADE_INST_POS_S, 3, yVal);

    if (sqlite3_step (FACADE_INST_POS_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Error while updating facade position values."));
        return -1;
    }

    return 0;
}


static const char PAN_PATCH_SPACE[] =
    "UPDATE ScenePatchSpace SET panX=?2,panY=?3,scale=?4 WHERE id=?1";
static sqlite3_stmt* PAN_PATCH_SPACE_S;

int
lsddb_panPatchSpace (int psId, int xVal, int yVal, double scale)
{
    sqlite3_reset (PAN_PATCH_SPACE_S);
    sqlite3_bind_int (PAN_PATCH_SPACE_S, 1, psId);
    sqlite3_bind_int (PAN_PATCH_SPACE_S, 2, xVal);
    sqlite3_bind_int (PAN_PATCH_SPACE_S, 3, yVal);
    sqlite3_bind_double (PAN_PATCH_SPACE_S, 4, scale);

    if (sqlite3_step (PAN_PATCH_SPACE_S) != SQLITE_DONE)
        return -1;

    return 0;
}


/* Plugin Related operations below */

/* index.html generator */
#include <evhttp.h>

static const char INDEX_HTML_GEN[] =
    "SELECT pluginDomain,id FROM ScenePlugin WHERE enabled=1 OR id=1";
static sqlite3_stmt* INDEX_HTML_GEN_S;

static const char INDEX_HTML_HEAD[] =
    "<!DOCTYPE html>\n"
    "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
    "\t<head>\n"
    "\t\t<title>LightShoppe Client</title>\n"
    "\t\t<meta charset=\"utf-8\"/>\n"
    "\t\t<meta name = \"viewport\" content = \"user-scalable=no, initial-scale=1.0, width=device-width\">\n"
    "\t\t<link rel=\"icon\" href=\"../favicon.ico\" type=\"image/x-icon\" />\n"
    "\t\t<link rel=\"shortcut icon\" href=\"../favicon.ico\" type=\"image/x-icon\" />\n"
    "\t\t<script type=\"text/javascript\">var LSD_PLUGIN_TABLE = new Array();</script>\n"
    "\t\t<link rel=\"stylesheet\" type=\"text/css\" href=\"../jquery-ui/themes/custom-theme/jquery-ui-1.8.12.custom.css\">\n"
    "\t\t<script type=\"text/javascript\" src=\"../jquery/jquery.min.js\"></script>\n"
    "\t\t<script type=\"text/javascript\" src=\"../jquery-ui/jquery-ui.min.js\"></script>\n"
    "\n\t\t<!-- BEGIN PROCEDURALLY GENERATED CONTENT -->\n\n";

static const char INDEX_HTML_FOOT[] =
    "\n\t\t<!-- END PROCEDURALLY GENERATED CONTENT -->\n\n"
    "\t\t<link rel=\"stylesheet\" type=\"text/css\" href=\"../LSDClient.css\" />\n"
    "\t\t<link rel=\"stylesheet\" type=\"text/css\" href=\"../imageuploader/fileuploader.css\" />\n"
    "\t\t<link rel=\"stylesheet\" type=\"text/css\" href=\"../farbtastic/farbtastic.css\" />\n"
    "\t\t<script type=\"text/javascript\" src=\"../LSDClientIF.js\"></script>\n"
    "\t\t<script type=\"text/javascript\" src=\"../LSDClient.js\"></script>\n"
    "\t\t<script type=\"text/javascript\" src=\"../imageuploader/fileuploader.js\"></script>\n"
    "\t\t<script type=\"text/javascript\" src=\"../farbtastic/farbtastic.js\"></script>\n"
    "\t</head>\n"
    "\t<body>\n"
    "\t\t<div id=\"bgClip\"><div id=\"bg\"></div></div>\n"
    "\t\t<div id=\"canv\"></div>\n"
    "\t</body>\n"
    "</html>\n";

int
lsddb_indexHtmlGen (struct evbuffer* target)
{
    if (!target)
        return -1;

    /* Add head of html template */
    evbuffer_add_printf (target, "%s", INDEX_HTML_HEAD);

    /* Get plugin includes */
    sqlite3_reset (INDEX_HTML_GEN_S);
    while (sqlite3_step (INDEX_HTML_GEN_S) == SQLITE_ROW)
    {
        const unsigned char* pluginDirName = sqlite3_column_text (
            INDEX_HTML_GEN_S,
            0);
        int pluginId = sqlite3_column_int (INDEX_HTML_GEN_S, 1);
        if (pluginDirName)
        {
            getPluginWebIncludes (target,
                                  (const char*)pluginDirName);
            evbuffer_add_printf (
                target,
                "\t\t<script type=\"text/javascript\">\n\t\t\tLSD_PLUGIN_TABLE[%d] = "
                "%s_CoreHead;\n\t\t</script>\n",
                pluginId,
                (const char*)pluginDirName);
        }
    }

    /* Add foot of html template */
    evbuffer_add_printf (target, "%s", INDEX_HTML_FOOT);

    return 0;
}


/* List Plugins */
static const char JSON_PLUGINS[] =
    "SELECT id,pluginDomain,pluginSha,enabled FROM ScenePlugin WHERE seen=1 AND id!=1";
static sqlite3_stmt* JSON_PLUGINS_S;

int
lsddb_jsonPlugins (cJSON* target)
{
    if (!target)
        return -1;

    cJSON* pluginArr = cJSON_CreateArray ();

    sqlite3_reset (JSON_PLUGINS_S);

    while (sqlite3_step (JSON_PLUGINS_S) == SQLITE_ROW)
    {
        cJSON* pluginObj = cJSON_CreateObject ();

        int pluginId = sqlite3_column_int (JSON_PLUGINS_S, 0);
        const unsigned char* pluginDirName = sqlite3_column_text (
            JSON_PLUGINS_S,
            1);
        const unsigned char* pluginSha = sqlite3_column_text (JSON_PLUGINS_S, 2);
        int enabled = sqlite3_column_int (JSON_PLUGINS_S, 3);

        cJSON_AddNumberToObject (pluginObj, "pluginId", pluginId);
        cJSON_AddStringToObject (pluginObj,
                                 "pluginDir",
                                 (const char*)pluginDirName);
        cJSON_AddStringToObject (pluginObj, "pluginSha", (const char*)pluginSha);
        cJSON_AddNumberToObject (pluginObj, "enabled", enabled);

        cJSON_AddItemToArray (pluginArr, pluginObj);
    }

    cJSON_AddItemToObject (target, "plugins", pluginArr);

    return 0;
}


/* Disable Plugin */
static const char DISABLE_PLUGIN[] =
    "UPDATE ScenePlugin SET enabled=0 WHERE id=?1";
static sqlite3_stmt* DISABLE_PLUGIN_S;

int
lsddb_disablePlugin (int pluginId)
{
    sqlite3_reset (DISABLE_PLUGIN_S);
    sqlite3_bind_int (DISABLE_PLUGIN_S, 1, pluginId);
    if (sqlite3_step (DISABLE_PLUGIN_S) == SQLITE_DONE)
        return 0;
    return -1;
}


/* Enable Plugin */
static const char ENABLE_PLUGIN[] =
    "UPDATE ScenePlugin SET enabled=1 WHERE id=?1";
static sqlite3_stmt* ENABLE_PLUGIN_S;

int
lsddb_enablePlugin (int pluginId)
{
    sqlite3_reset (ENABLE_PLUGIN_S);
    sqlite3_bind_int (ENABLE_PLUGIN_S, 1, pluginId);
    if (sqlite3_step (ENABLE_PLUGIN_S) == SQLITE_DONE)
        return 0;
    return -1;
}


/* Remove all traces of a plugin given its id */
/* Removes plugin, classes, nodes, types, tables */

static const char PURGE_PLUGIN_SELECT_CLASS[] =
    "";

int
lsddb_purgePlugin (int pluginId)
{

    /* First disable plugin */
    lsddb_disablePlugin (pluginId);

    /* remove nodes */

    /* remove classes */
    /* remove types */
    /* remove tables */
    /* remove plugin */
    return 0;
}


/* Plugin HEAD loader - ensures HEAD (extracted from SOs
 * iteratively) exists in DB, adds if not, */
/* inits plugin if 'enabled' is true */
static const char PLUGIN_HEAD_LOADER_CHECK_NAME[] =
    "SELECT id,enabled,loaded FROM ScenePlugin WHERE pluginDomain=?1 LIMIT 1";
static sqlite3_stmt* PLUGIN_HEAD_LOADER_CHECK_NAME_S;

static const char PLUGIN_HEAD_LOADER_CHECK_SHA[] =
    "SELECT enabled FROM ScenePlugin WHERE id=?1 AND pluginSha=?2 LIMIT 1";
static sqlite3_stmt* PLUGIN_HEAD_LOADER_CHECK_SHA_S;

static const char PLUGIN_HEAD_LOADER_UPDATE_SHA[] =
    "UPDATE ScenePlugin SET pluginSha=?2 WHERE id=?1";
static sqlite3_stmt* PLUGIN_HEAD_LOADER_UPDATE_SHA_S;

static const char PLUGIN_HEAD_LOADER_SEEN[] =
    "UPDATE ScenePlugin SET seen=1 WHERE id=?1";
static sqlite3_stmt* PLUGIN_HEAD_LOADER_SEEN_S;

static const char PLUGIN_HEAD_LOADER_INSERT[] =
    "INSERT INTO ScenePlugin (pluginDomain,pluginSha,loaded,enabled,seen) VALUES (?1,?2,0,0,1)";
static sqlite3_stmt* PLUGIN_HEAD_LOADER_INSERT_S;

static const char PLUGIN_HEAD_LOADER_UPDIDX_LOAD[] =
    "UPDATE ScenePlugin SET arrayIdx=?1,loaded=1,seen=1 WHERE id=?2";
static sqlite3_stmt* PLUGIN_HEAD_LOADER_UPDIDX_LOAD_S;

int
lsddb_pluginHeadLoader (ghType phGet,
                        int enable,
                        const char* parentDirectoryName,
                        const char* pluginSha,
                        lt_dlhandle dlObj)
{
    if (!phGet || !pluginSha)
    {
        doLog (ERROR, LOG_COMP, _("Invalid use of pluginHeadLoader()."));
        return -1;
    }

    sqlite3_reset (PLUGIN_HEAD_LOADER_CHECK_NAME_S);
    sqlite3_bind_text (PLUGIN_HEAD_LOADER_CHECK_NAME_S,
                       1,
                       parentDirectoryName,
                       -1,
                       NULL);
    doLog (NOTICE, LOG_COMP, _("Checking %s."), parentDirectoryName);

    int pluginId;
    int enabled = 0;
    int loaded = 0;
    if (sqlite3_step (PLUGIN_HEAD_LOADER_CHECK_NAME_S) == SQLITE_ROW)
    {
        /* Found by name */
        doLog (NOTICE, LOG_COMP, _("Already in DB."));
        pluginId = sqlite3_column_int (PLUGIN_HEAD_LOADER_CHECK_NAME_S, 0);
        enabled = sqlite3_column_int (PLUGIN_HEAD_LOADER_CHECK_NAME_S, 1);
        loaded = sqlite3_column_int (PLUGIN_HEAD_LOADER_CHECK_NAME_S, 2);
        
        /* If plugin is already loaded, don't load this one to prevent clashes */
        if (loaded)
        {
            doLog (WARNING, LOG_COMP, 
                   _("A plugin with the name '%s' was already loaded;\n"
                     "Skipping this one with SHA '%s'."),
                   parentDirectoryName, pluginSha);
            
            return -1;
        }
        

        /* If enabled, verify SHA1 matches last known SHA1 */
        if (enabled)
        {
            sqlite3_reset (PLUGIN_HEAD_LOADER_CHECK_SHA_S);
            sqlite3_bind_int (PLUGIN_HEAD_LOADER_CHECK_SHA_S, 1, pluginId);
            sqlite3_bind_text (PLUGIN_HEAD_LOADER_CHECK_SHA_S,
                               2,
                               pluginSha,
                               40,
                               NULL);
            if (sqlite3_step (PLUGIN_HEAD_LOADER_CHECK_SHA_S) != SQLITE_ROW)
            {
                /* Match not found, disable and update */
                enabled = 0;
                lsddb_disablePlugin (pluginId);

                sqlite3_reset (PLUGIN_HEAD_LOADER_UPDATE_SHA_S);
                sqlite3_bind_int (PLUGIN_HEAD_LOADER_UPDATE_SHA_S, 1, pluginId);
                sqlite3_bind_text (PLUGIN_HEAD_LOADER_UPDATE_SHA_S,
                                   2,
                                   pluginSha,
                                   40,
                                   NULL);
                if (sqlite3_step (PLUGIN_HEAD_LOADER_UPDATE_SHA_S) !=
                    SQLITE_DONE)
                    doLog (ERROR, LOG_COMP, _("Error while updating plugin SHA1."));
            }
        }

        sqlite3_reset (PLUGIN_HEAD_LOADER_SEEN_S);
        sqlite3_bind_int (PLUGIN_HEAD_LOADER_SEEN_S, 1, pluginId);

        if (sqlite3_step (PLUGIN_HEAD_LOADER_SEEN_S) != SQLITE_DONE)
        {
            doLog (ERROR, LOG_COMP, _("Unable to mark plugin seen in pluginHeadLoader()."));
            return -1;
        }
    }
    else  /* We must add new plugin record to DB */
    {
        doLog (NOTICE, LOG_COMP, _("Adding %s to DB."), parentDirectoryName);
        sqlite3_reset (PLUGIN_HEAD_LOADER_INSERT_S);
        sqlite3_bind_text (PLUGIN_HEAD_LOADER_INSERT_S,
                           1,
                           parentDirectoryName,
                           -1,
                           NULL);
        sqlite3_bind_text (PLUGIN_HEAD_LOADER_INSERT_S, 2, pluginSha, 40, NULL);

        if (sqlite3_step (PLUGIN_HEAD_LOADER_INSERT_S) != SQLITE_DONE)
        {
            doLog (ERROR, LOG_COMP, _("Unable to insert newfound plugin into DB in pluginHeadLoader()\nDetails: %s"),
                   sqlite3_errmsg (memdb));
            return -1;
        }
        if (enable)
            pluginId = sqlite3_last_insert_rowid (memdb);
            /* printf("Plugin ID: %d\n",pluginId); */
            /* enabled = 1; */

        /* return 0; */
    }

    if (enabled || enable)
    {
        /* Cleared to allocate plugin and execute it's init function */
        size_t pluginArrIdx;
        struct LSD_ScenePlugin* scenePlugin;
        if (insertElem (getArr_lsdPluginArr (), &pluginArrIdx,
                        (void**)&scenePlugin) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to insert plugin into array in pluginHeadLoader()."));
            return -1;
        }

        scenePlugin->dbId = pluginId;

        const struct LSD_ScenePluginHEAD* ph = phGet ();
        if (!ph || !ph->initFunc || !ph->cleanupFunc)
        {
            doLog (ERROR, LOG_COMP, _("Invalid Plugin HEAD extracted from %s."),
                     parentDirectoryName);
            return -1;
        }

        if (ph->initFunc (scenePlugin) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Plugin's own init failed within pluginHeadLoader()."));
            return -1;
        }

        /* Copy plugin destructor to plugin array for future
         * use */
        scenePlugin->cleanupFunc = ph->cleanupFunc;
        scenePlugin->handleRPC = ph->rpcFunc;

        /* Copy dlObj to be able to close SO */
        scenePlugin->dlObj = dlObj;

        /* Now that the plugin is allocated and inited, it's
         * loaded status and arrIdx is updated */
        sqlite3_reset (PLUGIN_HEAD_LOADER_UPDIDX_LOAD_S);
        sqlite3_bind_int (PLUGIN_HEAD_LOADER_UPDIDX_LOAD_S, 1, pluginArrIdx);
        sqlite3_bind_int (PLUGIN_HEAD_LOADER_UPDIDX_LOAD_S, 2, pluginId);

        if (sqlite3_step (PLUGIN_HEAD_LOADER_UPDIDX_LOAD_S) != SQLITE_DONE)
        {
            doLog (ERROR, LOG_COMP, _("Error updating plugin's status in DB in pluginHeadLoader()."));
            return -1;
        }

    }
    else
    if (dlObj)
        lt_dlclose (dlObj);

    return 0;

}


static const char RESOLVE_PLUGIN_FROM_NODE[] =
    "SELECT ScenePlugin.arrayIdx FROM ScenePlugin,SceneNodeClass,SceneNodeInst WHERE "
    "ScenePlugin.id=SceneNodeClass.pluginId AND SceneNodeClass.id=SceneNodeInst.classId AND SceneNodeInst.id=?1";
static sqlite3_stmt* RESOLVE_PLUGIN_FROM_NODE_S;

int
lsddb_resolvePluginFromNodeId (struct LSD_ScenePlugin** pluginBind, int nodeId)
{
    sqlite3_reset (RESOLVE_PLUGIN_FROM_NODE_S);
    sqlite3_bind_int (RESOLVE_PLUGIN_FROM_NODE_S, 1, nodeId);

    if (sqlite3_step (RESOLVE_PLUGIN_FROM_NODE_S) == SQLITE_ROW)
    {
        int pluginArrayIdx = sqlite3_column_int (RESOLVE_PLUGIN_FROM_NODE_S, 0);

        if (pickIdx (getArr_lsdPluginArr (), (void**)pluginBind,
                     pluginArrayIdx) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to pick plugin from array in resolvePluginFromNodeId()."));
            return -1;
        }
    }
    else
    {
        doLog (ERROR, LOG_COMP, _("Unable to resolve plugin id in resolvePluginFromNodeId()."));
        return -1;
    }

    return 0;
}


static const char TRACE_INPUT[] =
    "SELECT facadeBool,aliasedIn,arrIdx,typeId,id FROM SceneNodeInstInput WHERE id=?1";
static sqlite3_stmt* TRACE_INPUT_S;

int
lsddb_traceInput (struct LSD_SceneNodeInput** ptrToBind,
                  int inputId,
                  int* typeIdBind,
                  int* tracedIn)
{
    /* Recursively drill down to find a standard node's
     * input (i.e. not a facade input) */
    int facadeBool = 1;
    int aliasedIn = inputId;

    while (facadeBool)
    {
        sqlite3_reset (TRACE_INPUT_S);
        sqlite3_bind_int (TRACE_INPUT_S, 1, aliasedIn);
        if (sqlite3_step (TRACE_INPUT_S) == SQLITE_ROW)
        {
            facadeBool = sqlite3_column_int (TRACE_INPUT_S, 0);
            aliasedIn = sqlite3_column_int (TRACE_INPUT_S, 1);
        }
        else
        {
            doLog (ERROR, LOG_COMP, _("Unable to traceInput %d()\nDetails: %s"), 
                   inputId, sqlite3_errmsg (memdb));
            return -1;
        }
    }
    /* Finally done */
    int arrIdx = sqlite3_column_int (TRACE_INPUT_S, 2);
    if (arrIdx == -1)
    {
        doLog (WARNING, LOG_COMP, _("Input is not constructed in memory; its plugin may be disabled."));
        return -1;
    }

    if (tracedIn)
        *tracedIn = sqlite3_column_int (TRACE_INPUT_S, 4);

    if (typeIdBind)
        *typeIdBind = sqlite3_column_int (TRACE_INPUT_S, 3);

    if (!ptrToBind)
        return 0;

    /* Bind input object for caller */
    struct LSD_SceneNodeInput* inputObj;
    if (pickIdx (getArr_lsdNodeInputArr (), (void**)&inputObj, arrIdx) < 0)
    {
        doLog (ERROR, LOG_COMP, _("Unable to pick Input in traceInput()."));
        return -1;
    }

    *ptrToBind = inputObj;

    return 0;
}


static const char TRACE_OUTPUT[] =
    "SELECT facadeBool,aliasedOut,arrIdx,typeId,id FROM SceneNodeInstOutput WHERE id=?1";
static sqlite3_stmt* TRACE_OUTPUT_S;

int
lsddb_traceOutput (struct LSD_SceneNodeOutput** ptrToBind,
                   int outputId,
                   int* typeIdBind,
                   int* tracedOut)
{
    /* Recursively drill down to find a standard node's
     * output (i.e. not a facade output) */
    int facadeBool = 1;
    int aliasedOut = outputId;

    while (facadeBool)
    {
        sqlite3_reset (TRACE_OUTPUT_S);
        sqlite3_bind_int (TRACE_OUTPUT_S, 1, aliasedOut);
        if (sqlite3_step (TRACE_OUTPUT_S) == SQLITE_ROW)
        {
            facadeBool = sqlite3_column_int (TRACE_OUTPUT_S, 0);
            aliasedOut = sqlite3_column_int (TRACE_OUTPUT_S, 1);
        }
        else
            /* fprintf(stderr,"Unable to traceOutput()\n"); */
            return -1;
    }
    /* Finally done */
    int arrIdx = sqlite3_column_int (TRACE_OUTPUT_S, 2);
    if (arrIdx == -1)
    {
        doLog (WARNING, LOG_COMP, _("Output is not constructed in memory; its plugin may be disabled."));
        return -1;
    }

    if (tracedOut)
        *tracedOut = sqlite3_column_int (TRACE_OUTPUT_S, 4);

    if (typeIdBind)
        *typeIdBind = sqlite3_column_int (TRACE_OUTPUT_S, 3);

    if (!ptrToBind)
        return 0;

    /* Bind output object for caller */
    struct LSD_SceneNodeOutput* outputObj;
    if (pickIdx (getArr_lsdNodeOutputArr (), (void**)&outputObj, arrIdx) < 0)
    {
        doLog (ERROR, LOG_COMP, _("Unable to pick Output in traceOutput()."));
        return -1;
    }

    *ptrToBind = outputObj;

    return 0;
}


static const char CHECK_CHANNEL_WIRING_GET_PS[] =
    "SELECT ScenePatchSpace.parentPatchSpace FROM ScenePatchSpace,SceneNodeInstOutput "
    "WHERE SceneNodeInstOutput.instId=ScenePatchSpace.id AND SceneNodeInstOutput.facadeBool=1 "
    "AND SceneNodeInstOutput.id=?1";
static sqlite3_stmt* CHECK_CHANNEL_WIRING_GET_PS_S;

static const char CHECK_CHANNEL_WIRING_GET_CHAN_ARRIDX[] =
    "SELECT arrIdx FROM SystemChannel WHERE facadeOutId=?1";
static sqlite3_stmt* CHECK_CHANNEL_WIRING_GET_CHAN_ARRIDX_S;

int
lsddb_checkChannelWiring (int facadeOutId, int srcOut)
{
    /* from srcOut=2 to facadeOut=1 */
    /* printf("Checking to wire channel from %d (traceroot)
     * to %d\n",srcOut,facadeOutId); */

    sqlite3_reset (CHECK_CHANNEL_WIRING_GET_PS_S);
    sqlite3_bind_int (CHECK_CHANNEL_WIRING_GET_PS_S, 1, facadeOutId);
    if (sqlite3_step (CHECK_CHANNEL_WIRING_GET_PS_S) == SQLITE_ROW)
    {
        int psId = sqlite3_column_int (CHECK_CHANNEL_WIRING_GET_PS_S, 0);

        if (psId == 0)  /* This is a channel output, make
                         * connection */

        {
            sqlite3_reset (CHECK_CHANNEL_WIRING_GET_CHAN_ARRIDX_S);
            sqlite3_bind_int (CHECK_CHANNEL_WIRING_GET_CHAN_ARRIDX_S,
                              1,
                              facadeOutId);
            if (sqlite3_step (CHECK_CHANNEL_WIRING_GET_CHAN_ARRIDX_S) ==
                SQLITE_ROW)
            {
                int chanArrIdx = sqlite3_column_int (
                    CHECK_CHANNEL_WIRING_GET_CHAN_ARRIDX_S,
                    0);

                /* Pick the channel */
                struct LSD_Channel* chan;
                if (pickIdx (getArr_lsdChannelArr (), (void**)&chan,
                             chanArrIdx) < 0)
                {
                    doLog (ERROR, LOG_COMP, _("Unable to pick channel from array in checkChannelWiring()."));
                    return -1;
                }

                /* Trace the output and connect on channel */
                chan->output = NULL;
                lsddb_traceOutput (&( chan->output ), srcOut, NULL, NULL);

            }
            else
            {
                doLog (ERROR, LOG_COMP, _("Couldn't resolve arrIdx from facadeOut on root patchSpace."));
                return -1;
            }

        }
    }
    else
    {
        doLog (ERROR, LOG_COMP, _("Unable to retrieve parentPatchSpace from facadeOut."));
        return -1;
    }
    return 0;
}


int
lsddb_checkChannelUnwiring (int facadeOutId)
{
    sqlite3_reset (CHECK_CHANNEL_WIRING_GET_PS_S);
    sqlite3_bind_int (CHECK_CHANNEL_WIRING_GET_PS_S, 1, facadeOutId);
    if (sqlite3_step (CHECK_CHANNEL_WIRING_GET_PS_S) == SQLITE_ROW)
    {
        int psId = sqlite3_column_int (CHECK_CHANNEL_WIRING_GET_PS_S, 0);

        if (psId == 0)  /* This is a channel output,
                         * disconnect */

        {
            sqlite3_reset (CHECK_CHANNEL_WIRING_GET_CHAN_ARRIDX_S);
            sqlite3_bind_int (CHECK_CHANNEL_WIRING_GET_CHAN_ARRIDX_S,
                              1,
                              facadeOutId);
            if (sqlite3_step (CHECK_CHANNEL_WIRING_GET_CHAN_ARRIDX_S) ==
                SQLITE_ROW)
            {
                int chanArrIdx = sqlite3_column_int (
                    CHECK_CHANNEL_WIRING_GET_CHAN_ARRIDX_S,
                    0);

                /* Pick the channel */
                struct LSD_Channel* chan;
                if (pickIdx (getArr_lsdChannelArr (), (void**)&chan,
                             chanArrIdx) < 0)
                {
                    doLog (ERROR, LOG_COMP, _("Unable to pick channel from array in checkChannelUnwiring()."));
                    return -1;
                }

                /* Disconnect on channel */
                chan->output = NULL;

            }
            else
            {
                doLog (ERROR, LOG_COMP, _("Couldn't resolve arrIdx from facadeOut on root patchSpace."));
                return -1;
            }

        }
    }
    else
    {
        doLog (ERROR, LOG_COMP, _("Unable to retrieve parentPatchSpace from facadeOut."));
        return -1;
    }
    return 0;
}


/* Statement to ensure that no two wires are connected to
 * facade interior out */
static const char WIRE_NODES_CHECK_FACADE_INT_OUT[] =
    "SELECT id FROM SceneNodeEdge WHERE destIn=?1 AND destFacadeInt=1";
static sqlite3_stmt* WIRE_NODES_CHECK_FACADE_INT_OUT_S;

/* data for various verification and updating tasks */
static const char WIRE_NODES_CHECK_SRC_OUT[] =
    "SELECT typeId,facadeBool,aliasedOut FROM SceneNodeInstOutput WHERE id=?1";
static sqlite3_stmt* WIRE_NODES_CHECK_SRC_OUT_S;

static const char WIRE_NODES_CHECK_DEST_IN[] =
    "SELECT typeId,facadeBool,aliasedIn FROM SceneNodeInstInput WHERE id=?1";
static sqlite3_stmt* WIRE_NODES_CHECK_DEST_IN_S;

static const char WIRE_NODES_SET_FACADE_IN_DATA[] =
    "UPDATE SceneNodeInstInput SET typeId=?2,aliasedIn=?3 WHERE id=?1";
static sqlite3_stmt* WIRE_NODES_SET_FACADE_IN_DATA_S;

static const char WIRE_NODES_SET_FACADE_OUT_DATA[] =
    "UPDATE SceneNodeInstOutput SET typeId=?2,aliasedOut=?3 WHERE id=?1";
static sqlite3_stmt* WIRE_NODES_SET_FACADE_OUT_DATA_S;

/* Four statements below used to ensure same patch space and
 * both node's plugins enabled */
static const char WIRE_NODES_GET_FACADE_IN_CPS[] =
    "SELECT SceneNodeInstInput.instId,ScenePatchSpace.parentPatchSpace FROM SceneNodeInstInput,ScenePatchSpace WHERE "
    "SceneNodeInstInput.instId=ScenePatchSpace.id AND SceneNodeInstInput.id=?1 AND SceneNodeInstInput.facadeBool=1";
static sqlite3_stmt* WIRE_NODES_GET_FACADE_IN_CPS_S;

static const char WIRE_NODES_GET_FACADE_OUT_CPS[] =
    "SELECT SceneNodeInstOutput.instId,ScenePatchSpace.parentPatchSpace FROM SceneNodeInstOutput,ScenePatchSpace WHERE "
    "SceneNodeInstOutput.instId=ScenePatchSpace.id AND SceneNodeInstOutput.id=?1 AND SceneNodeInstOutput.facadeBool=1";
static sqlite3_stmt* WIRE_NODES_GET_FACADE_OUT_CPS_S;

static const char WIRE_NODES_GET_IN_PS[] =
    "SELECT SceneNodeInst.patchSpaceId,SceneNodeInstInput.typeId,SceneNodeInst.classId "
    "FROM SceneNodeInst,SceneNodeInstInput "
    "WHERE SceneNodeInst.id=SceneNodeInstInput.instId AND SceneNodeInstInput.id=?1 "
    "AND SceneNodeInstInput.facadeBool=0";
static sqlite3_stmt* WIRE_NODES_GET_IN_PS_S;

static const char WIRE_NODES_GET_OUT_PS[] =
    "SELECT SceneNodeInst.patchSpaceId,SceneNodeInstOutput.typeId,SceneNodeInst.classId "
    "FROM SceneNodeInst,SceneNodeInstOutput "
    "WHERE SceneNodeInst.id=SceneNodeInstOutput.instId AND SceneNodeInstOutput.id=?1 "
    "AND SceneNodeInstOutput.facadeBool=0";
static sqlite3_stmt* WIRE_NODES_GET_OUT_PS_S;

static const char WIRE_NODES[] =
    "INSERT INTO SceneNodeEdge (srcFacadeInt,srcOut,destFacadeInt,destIn,patchSpaceId) VALUES (?1,?2,?3,?4,?5)";
static sqlite3_stmt* WIRE_NODES_S;

int
lsddb_wireNodes (int srcFacadeInt,
                 int srcId,
                 int destFacadeInt,
                 int destId,
                 int* idBinding)
{

    /* Connecting an interior facade in and out */
    /* Redundant and disallowed to avoid recursion bugs */
    if (srcFacadeInt && destFacadeInt)
    {
        doLog (ERROR, LOG_COMP, _("Redundant connections not allowed."));
        return -1;
    }

    /* Ensure nothing is already plugged into the
     * destination */
    if (destFacadeInt)
    {
        /* Destination is a facade. Find wires */
        sqlite3_reset (WIRE_NODES_CHECK_FACADE_INT_OUT_S);
        sqlite3_bind_int (WIRE_NODES_CHECK_FACADE_INT_OUT_S, 1, destId);
        if (sqlite3_step (WIRE_NODES_CHECK_FACADE_INT_OUT_S) == SQLITE_ROW)
        {
            doLog (ERROR, LOG_COMP, _("Attempting to connect wire to already connected facade interior out."));
            return -1;
        }
    }
    else
    {
        /* Destination is a node. Use this stmt to find any
         * wires */
        sqlite3_reset (REMOVE_NODE_INST_INPUT_GET_WIRES_S);
        sqlite3_bind_int (REMOVE_NODE_INST_INPUT_GET_WIRES_S, 1, destId);
        if (sqlite3_step (REMOVE_NODE_INST_INPUT_GET_WIRES_S) == SQLITE_ROW)
        {
            doLog (ERROR, LOG_COMP, _("Attempting to connect a wire to an already connected input."));
            return -1;
        }
    }

    int srcPS;
    int destPS;
    int srcClass = -1;
    int destClass = -1;

    /* holder of wire ID for after insertion */
    int wireId;

    if (srcFacadeInt)  /* Left of wire is attached to
                        * interior of facade (technically an
                        * input) */

    {   /* First ensure that the interior of the facade
         * input is */
        /* within the same patchSpace as the input being
         * connected to */

        /* Src */
        sqlite3_reset (WIRE_NODES_GET_FACADE_IN_CPS_S);
        sqlite3_bind_int (WIRE_NODES_GET_FACADE_IN_CPS_S, 1, srcId);
        if (sqlite3_step (WIRE_NODES_GET_FACADE_IN_CPS_S) == SQLITE_ROW)
            srcPS = sqlite3_column_int (WIRE_NODES_GET_FACADE_IN_CPS_S, 0);
        else
        {
            doLog (ERROR, LOG_COMP, _("Unable to verify facade input's patch space."));
            return -1;
        }

        /* Dest */
        sqlite3_reset (WIRE_NODES_GET_IN_PS_S);
        sqlite3_bind_int (WIRE_NODES_GET_IN_PS_S, 1, destId);
        if (sqlite3_step (WIRE_NODES_GET_IN_PS_S) == SQLITE_ROW)
        {
            destPS = sqlite3_column_int (WIRE_NODES_GET_IN_PS_S, 0);
            destClass = sqlite3_column_int (WIRE_NODES_GET_IN_PS_S, 2);
        }
        else
        {
            doLog (ERROR, LOG_COMP, _("Unable to verify node input's patch space."));
            return -1;
        }

        if (srcPS != destPS)
        {
            doLog (ERROR, LOG_COMP, _("Patch Spaces Do not match in wireNodes()."));
            return -1;
        }

        if (!lsddb_checkClassEnabled (destClass))
        {
            doLog (ERROR, LOG_COMP, _("Unable to connect wire's destination; destination class disabled."));
            return -1;
        }

        /* Get data of wire src to verify that it actually
         * is a facade plug (the afformentioned input) */
        sqlite3_reset (WIRE_NODES_CHECK_DEST_IN_S);
        sqlite3_bind_int (WIRE_NODES_CHECK_DEST_IN_S, 1, srcId);
        int checkFacadeBool = 0;
        if (sqlite3_step (WIRE_NODES_CHECK_DEST_IN_S) == SQLITE_ROW)
            checkFacadeBool = sqlite3_column_int (WIRE_NODES_CHECK_DEST_IN_S, 1);
        if (!checkFacadeBool)
        {
            doLog (ERROR, LOG_COMP, _("Wire source does not check to be a facade plug as claimed in wireNodes()."));
            return -1;
        }

        /* Get data of wire dest (inside facade) */
        sqlite3_reset (WIRE_NODES_CHECK_DEST_IN_S);
        sqlite3_bind_int (WIRE_NODES_CHECK_DEST_IN_S, 1, destId);
        if (sqlite3_step (WIRE_NODES_CHECK_DEST_IN_S) == SQLITE_ROW)
        {
            int typeId = sqlite3_column_int (WIRE_NODES_CHECK_DEST_IN_S, 0);
            /* int facadeBool =
             * sqlite3_column_int(WIRE_NODES_CHECK_DEST_IN_S,1); */
            /* int aliasedIn =
             * sqlite3_column_int(WIRE_NODES_CHECK_DEST_IN_S,2); */

            if (typeId < 0)
            {
                doLog (ERROR, LOG_COMP, _("Facade Plug not internally connected on input %d."),
                         destId);
                return -1;
            }

            /* set typeId and aliasedIn of src (still the
             * input) */
            sqlite3_reset (WIRE_NODES_SET_FACADE_IN_DATA_S);
            sqlite3_bind_int (WIRE_NODES_SET_FACADE_IN_DATA_S, 1, srcId);
            sqlite3_bind_int (WIRE_NODES_SET_FACADE_IN_DATA_S, 2, typeId);
            sqlite3_bind_int (WIRE_NODES_SET_FACADE_IN_DATA_S, 3, destId);

            if (sqlite3_step (WIRE_NODES_SET_FACADE_IN_DATA_S) != SQLITE_DONE)
            {
                doLog (ERROR, LOG_COMP, _("Unable to update facade plug data after internal connection in wireNodes()."));
                return -1;
            }

        }
        else
        {
            doLog (ERROR, LOG_COMP, _("Dest Input non-existent."));
            return -1;
        }

        /* Now the Edge can be added to DB */
        sqlite3_reset (WIRE_NODES_S);
        sqlite3_bind_int (WIRE_NODES_S, 1, 1);
        sqlite3_bind_int (WIRE_NODES_S, 2, srcId);
        sqlite3_bind_int (WIRE_NODES_S, 3, 0);
        sqlite3_bind_int (WIRE_NODES_S, 4, destId);
        sqlite3_bind_int (WIRE_NODES_S, 5, srcPS);

        if (sqlite3_step (WIRE_NODES_S) != SQLITE_DONE)
        {
            doLog (ERROR, LOG_COMP, _("Unable to insert wire into DB in wireNodes()."));
            return -1;
        }

        wireId = sqlite3_last_insert_rowid (memdb);

        if (idBinding)
            *idBinding = wireId;

        /* Done here (now able to wire facade input
         * externally) */
        return 0;
    }

    if (destFacadeInt)  /* Right of wire is attached to
                         * interior of facade (technically
                         * an output) */

    {   /* First ensure that the interior of the facade
         * output is */
        /* within the same patchSpace as the output being
         * connected to */

        /* Src */
        sqlite3_reset (WIRE_NODES_GET_OUT_PS_S);
        sqlite3_bind_int (WIRE_NODES_GET_OUT_PS_S, 1, srcId);
        if (sqlite3_step (WIRE_NODES_GET_OUT_PS_S) == SQLITE_ROW)
        {
            srcPS = sqlite3_column_int (WIRE_NODES_GET_OUT_PS_S, 0);
            srcClass = sqlite3_column_int (WIRE_NODES_GET_OUT_PS_S, 2);
        }
        else  /* Try connecting with facade src */
        {
            sqlite3_reset (WIRE_NODES_GET_FACADE_OUT_CPS_S);
            sqlite3_bind_int (WIRE_NODES_GET_FACADE_OUT_CPS_S, 1, srcId);
            if (sqlite3_step (WIRE_NODES_GET_FACADE_OUT_CPS_S) == SQLITE_ROW)
            {
                srcPS = sqlite3_column_int (WIRE_NODES_GET_FACADE_OUT_CPS_S, 1);
                srcClass = -1;
            }
            else
            {
                doLog (ERROR, LOG_COMP, _("Unable to verify node output's patch space."));
                return -1;
            }

        }

        /* Dest */
        sqlite3_reset (WIRE_NODES_GET_FACADE_OUT_CPS_S);
        sqlite3_bind_int (WIRE_NODES_GET_FACADE_OUT_CPS_S, 1, destId);
        if (sqlite3_step (WIRE_NODES_GET_FACADE_OUT_CPS_S) == SQLITE_ROW)
            destPS = sqlite3_column_int (WIRE_NODES_GET_FACADE_OUT_CPS_S, 0);
        else
        {
            doLog (ERROR, LOG_COMP, _("Unable to verify facade output's patch space."));
            return -1;
        }

        if (srcPS != destPS)
        {
            doLog (ERROR, LOG_COMP, _("Patch Spaces Do not match in wireNodes()."));
            return -1;
        }

        if (srcClass == -1)  /* Verify internal connection */
        {
            sqlite3_reset (WIRE_NODES_CHECK_SRC_OUT_S);
            sqlite3_bind_int (WIRE_NODES_CHECK_SRC_OUT_S, 1, srcId);
            int aliasedOut = -1;
            if (sqlite3_step (WIRE_NODES_CHECK_SRC_OUT_S) == SQLITE_ROW)
                aliasedOut = sqlite3_column_int (WIRE_NODES_CHECK_SRC_OUT_S, 2);
            if (aliasedOut <= 0)
            {
                doLog (ERROR, LOG_COMP, _("Unable to connect wire's source; not internally connected."));
                return -1;
            }
        }
        else if (!lsddb_checkClassEnabled (srcClass))
        {
            doLog (ERROR, LOG_COMP, _("Unable to connect wire's source; source class disabled."));
            return -1;
        }

        /* Get data of wire dest to verify that it actually
         * is a facade plug (the afformentioned output) */
        sqlite3_reset (WIRE_NODES_CHECK_SRC_OUT_S);
        sqlite3_bind_int (WIRE_NODES_CHECK_SRC_OUT_S, 1, destId);
        int checkFacadeBool = 0;
        if (sqlite3_step (WIRE_NODES_CHECK_SRC_OUT_S) == SQLITE_ROW)
            checkFacadeBool = sqlite3_column_int (WIRE_NODES_CHECK_SRC_OUT_S, 1);
        if (!checkFacadeBool)
        {
            doLog (ERROR, LOG_COMP, _("Wire source does not check to be a facade plug as claimed in wireNodes()."));
            return -1;
        }

        /* Get data of wire src (output inside facade) */
        sqlite3_reset (WIRE_NODES_CHECK_SRC_OUT_S);
        sqlite3_bind_int (WIRE_NODES_CHECK_SRC_OUT_S, 1, srcId);
        if (sqlite3_step (WIRE_NODES_CHECK_SRC_OUT_S) == SQLITE_ROW)
        {
            int typeId = sqlite3_column_int (WIRE_NODES_CHECK_SRC_OUT_S, 0);
            /* int facadeBool =
             * sqlite3_column_int(WIRE_NODES_CHECK_SRC_OUT_S,1); */
            /* int aliasedOut =
             * sqlite3_column_int(WIRE_NODES_CHECK_SRC_OUT_S,2); */

            if (typeId < 0)
            {
                doLog (ERROR, LOG_COMP, _("Facade Plug not internally connected on output %d."),
                         destId);
                return -1;
            }

            /* set typeId and aliasedOut of dest (still the
             * output) */
            sqlite3_reset (WIRE_NODES_SET_FACADE_OUT_DATA_S);
            sqlite3_bind_int (WIRE_NODES_SET_FACADE_OUT_DATA_S, 1, destId);
            sqlite3_bind_int (WIRE_NODES_SET_FACADE_OUT_DATA_S, 2, typeId);
            sqlite3_bind_int (WIRE_NODES_SET_FACADE_OUT_DATA_S, 3, srcId);

            if (sqlite3_step (WIRE_NODES_SET_FACADE_OUT_DATA_S) != SQLITE_DONE)
            {
                doLog (ERROR, LOG_COMP, _("Unable to update facade plug data after internal connection in wireNodes()."));
                return -1;
            }

            /* Facade output (srcId) MAY actually be a
             * partition's channel */
            /* This function checks this possibilility and
             * sets the correct channel pointers */
            lsddb_checkChannelWiring (destId, srcId);

        }
        else
        {
            doLog (ERROR, LOG_COMP, _("Dest Output non-existent."));
            return -1;
        }

        /* Now the Edge can be added to DB */
        sqlite3_reset (WIRE_NODES_S);
        sqlite3_bind_int (WIRE_NODES_S, 1, 0);
        sqlite3_bind_int (WIRE_NODES_S, 2, srcId);
        sqlite3_bind_int (WIRE_NODES_S, 3, 1);
        sqlite3_bind_int (WIRE_NODES_S, 4, destId);
        sqlite3_bind_int (WIRE_NODES_S, 5, srcPS);

        if (sqlite3_step (WIRE_NODES_S) != SQLITE_DONE)
        {
            doLog (ERROR, LOG_COMP, _("Unable to insert wire into DB in wireNodes()."));
            return -1;
        }

        wireId = sqlite3_last_insert_rowid (memdb);

        if (idBinding)
            *idBinding = wireId;

        /* Done here (now able to wire facade output
         * externally) */
        return 0;
    }

    /* First ensure that the src is */
    /* within the same patchSpace as the dest being
     * connected to */

    int srcType = -2;
    int destType = -2;

    /* Src */
    sqlite3_reset (WIRE_NODES_GET_OUT_PS_S);
    sqlite3_bind_int (WIRE_NODES_GET_OUT_PS_S, 1, srcId);
    if (sqlite3_step (WIRE_NODES_GET_OUT_PS_S) == SQLITE_ROW)
    {
        srcPS = sqlite3_column_int (WIRE_NODES_GET_OUT_PS_S, 0);
        srcType = sqlite3_column_int (WIRE_NODES_GET_OUT_PS_S, 1);
        srcClass = sqlite3_column_int (WIRE_NODES_GET_OUT_PS_S, 2);
    }
    else  /* Try testing as facade */

    {
        sqlite3_reset (WIRE_NODES_GET_FACADE_OUT_CPS_S);
        sqlite3_bind_int (WIRE_NODES_GET_FACADE_OUT_CPS_S, 1, srcId);
        if (sqlite3_step (WIRE_NODES_GET_FACADE_OUT_CPS_S) == SQLITE_ROW)
        {
            srcPS = sqlite3_column_int (WIRE_NODES_GET_FACADE_OUT_CPS_S, 1);
            int typeId;
            int tracedOut;
            if (lsddb_traceOutput (NULL, srcId, &typeId, &tracedOut) == 0)
            {
                srcType = typeId;
                srcClass = lsddb_getOutputClassId (tracedOut);
            }
        }
        else
        {
            doLog (ERROR, LOG_COMP, _("Unable to verify node output's patch space."));
            return -1;
        }
    }

    /* Dest */
    sqlite3_reset (WIRE_NODES_GET_IN_PS_S);
    sqlite3_bind_int (WIRE_NODES_GET_IN_PS_S, 1, destId);
    if (sqlite3_step (WIRE_NODES_GET_IN_PS_S) == SQLITE_ROW)
    {
        destPS = sqlite3_column_int (WIRE_NODES_GET_IN_PS_S, 0);
        destType = sqlite3_column_int (WIRE_NODES_GET_IN_PS_S, 1);
        destClass = sqlite3_column_int (WIRE_NODES_GET_IN_PS_S, 2);
    }
    else  /* Try testing as facade */

    {
        sqlite3_reset (WIRE_NODES_GET_FACADE_IN_CPS_S);
        sqlite3_bind_int (WIRE_NODES_GET_FACADE_IN_CPS_S, 1, destId);
        if (sqlite3_step (WIRE_NODES_GET_FACADE_IN_CPS_S) == SQLITE_ROW)
        {
            destPS = sqlite3_column_int (WIRE_NODES_GET_FACADE_IN_CPS_S, 1);
            int typeId;
            int tracedIn;
            if (lsddb_traceInput (NULL, destId, &typeId, &tracedIn) == 0)
            {
                destType = typeId;
                destClass = lsddb_getInputClassId (tracedIn);
            }
        }
        else
        {
            doLog (ERROR, LOG_COMP, _("Unable to verify node input's patch space."));
            return -1;
        }
    }

    if (srcPS != destPS)
    {
        doLog (ERROR, LOG_COMP, _("Patch Spaces Do not match in wireNodes()."));
        return -1;
    }

    if (srcType != destType)
    {
        doLog (ERROR, LOG_COMP, _("Types do not match in wireNodes()."));
        return -1;
    }

    if (!lsddb_checkClassEnabled (srcClass))
    {
        doLog (ERROR, LOG_COMP, _("Unable to connect wire's source; source class disabled."));
        return -1;
    }

    if (!lsddb_checkClassEnabled (destClass))
    {
        doLog (ERROR, LOG_COMP, _("Unable to connect wire's destination; destination class %d,%d disabled."),
            destId, destClass);
        return -1;
    }

    /* Use trace functions to resolve the source and
     * destination objects */
    struct LSD_SceneNodeOutput* src;
    struct LSD_SceneNodeInput* dest;

    if (lsddb_traceOutput (&src, srcId, NULL, NULL) < 0)
        return -1;
    if (lsddb_traceInput (&dest, destId, NULL, NULL) < 0)
        return -1;

    /* Now the Edge can be added to DB */
    sqlite3_reset (WIRE_NODES_S);
    sqlite3_bind_int (WIRE_NODES_S, 1, 0);
    sqlite3_bind_int (WIRE_NODES_S, 2, srcId);
    sqlite3_bind_int (WIRE_NODES_S, 3, 0);
    sqlite3_bind_int (WIRE_NODES_S, 4, destId);
    sqlite3_bind_int (WIRE_NODES_S, 5, srcPS);

    if (sqlite3_step (WIRE_NODES_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Unable to insert wire into DB in wireNodes()."));
        return -1;
    }

    wireId = sqlite3_last_insert_rowid (memdb);

    /* Perform pointer wiring */
    dest->connection = src;

    if (idBinding)
        *idBinding = wireId;

    return 0;
}


static const char UNWIRE_NODES_GET_EDGE_DETAILS[] =
    "SELECT srcFacadeInt,srcOut,destFacadeInt,destIn FROM SceneNodeEdge WHERE id=?1";
static sqlite3_stmt* UNWIRE_NODES_GET_EDGE_DETAILS_S;

static const char UNWIRE_NODES_DELETE_EDGE[] =
    "DELETE FROM SceneNodeEdge WHERE id=?1";
static sqlite3_stmt* UNWIRE_NODES_DELETE_EDGE_S;

static const char UNWIRE_NODES_GET_SRC_FACADE_EDGE[] =
    "SELECT id FROM SceneNodeEdge WHERE destIn=?1 AND destFacadeInt=0";
static sqlite3_stmt* UNWIRE_NODES_GET_SRC_FACADE_EDGE_S;

static const char UNWIRE_NODES_GET_DEST_FACADE_EDGE[] =
    "SELECT id FROM SceneNodeEdge WHERE srcOut=?1 AND srcFacadeInt=0";
static sqlite3_stmt* UNWIRE_NODES_GET_DEST_FACADE_EDGE_S;

static const char UNWIRE_FACADE_OUT_ALIAS[] =
    "UPDATE SceneNodeInstOutput SET aliasedOut=-1 WHERE id=?1";
static sqlite3_stmt* UNWIRE_FACADE_OUT_ALIAS_S;

int
lsddb_unwireNodes (int wireId)
{

    /* If either src or dest is an internal facade
     * connection, */
    /* the facade connention's wire is removed as well on
     * the exterior patchSpace */

    /* Start by retrieving the wire's details */
    sqlite3_reset (UNWIRE_NODES_GET_EDGE_DETAILS_S);
    sqlite3_bind_int (UNWIRE_NODES_GET_EDGE_DETAILS_S, 1, wireId);
    if (sqlite3_step (UNWIRE_NODES_GET_EDGE_DETAILS_S) == SQLITE_ROW)
    {
        int srcFacadeInt = sqlite3_column_int (UNWIRE_NODES_GET_EDGE_DETAILS_S,
                                               0);
        int srcOut = sqlite3_column_int (UNWIRE_NODES_GET_EDGE_DETAILS_S, 1);
        int destFacadeInt = sqlite3_column_int (UNWIRE_NODES_GET_EDGE_DETAILS_S,
                                                2);
        int destIn = sqlite3_column_int (UNWIRE_NODES_GET_EDGE_DETAILS_S, 3);

        if (srcFacadeInt)  /* Left side facade in */
        {   /* Remove wire connected outside facade in */
            sqlite3_reset (UNWIRE_NODES_GET_SRC_FACADE_EDGE_S);
            sqlite3_bind_int (UNWIRE_NODES_GET_SRC_FACADE_EDGE_S, 1, srcOut);
            if (sqlite3_step (UNWIRE_NODES_GET_SRC_FACADE_EDGE_S) == SQLITE_ROW)
            {
                int outsideEdge = sqlite3_column_int (
                    UNWIRE_NODES_GET_SRC_FACADE_EDGE_S,
                    0);
                lsddb_unwireNodes (outsideEdge);
            }
        }

        if (destFacadeInt)  /* Right side facade out */
        {   /* Facade output (destIn) MAY actually be a
             * partition's channel */
            /* This function checks this possibilility and
             * nullifies any channel pointers */
            lsddb_checkChannelUnwiring (destIn);

            /* Remove wire connected outside facade out */
            sqlite3_reset (UNWIRE_NODES_GET_DEST_FACADE_EDGE_S);
            sqlite3_bind_int (UNWIRE_NODES_GET_DEST_FACADE_EDGE_S, 1, destIn);
            if (sqlite3_step (UNWIRE_NODES_GET_DEST_FACADE_EDGE_S) ==
                SQLITE_ROW)
            {
                int outsideEdge = sqlite3_column_int (
                    UNWIRE_NODES_GET_DEST_FACADE_EDGE_S,
                    0);
                lsddb_unwireNodes (outsideEdge);
            }

            /* Remove interior alias */
            sqlite3_reset (UNWIRE_FACADE_OUT_ALIAS_S);
            sqlite3_bind_int (UNWIRE_FACADE_OUT_ALIAS_S, 1, destIn);
            sqlite3_step (UNWIRE_FACADE_OUT_ALIAS_S);
        }

        if (!srcFacadeInt && !destFacadeInt)  /* Facade
                                               * interiors
                                               * not
                                               * involved,
                                               * disconnect */
        {   /* Use trace functions to resolve the source and
             * destination objects */
            struct LSD_SceneNodeInput* dest;

            if (lsddb_traceInput (&dest, destIn, NULL, NULL) < 0)
            {
                doLog (ERROR, LOG_COMP, _("Unable to trace input for node disconnection in unwireNodes()."));
                return -1;
            }

            /* Disconnect pointer */
            dest->connection = NULL;
        }

        /* Remove the edge */
        sqlite3_reset (UNWIRE_NODES_DELETE_EDGE_S);
        sqlite3_bind_int (UNWIRE_NODES_DELETE_EDGE_S, 1, wireId);
        if (sqlite3_step (UNWIRE_NODES_DELETE_EDGE_S) != SQLITE_DONE)
        {
            doLog (ERROR, LOG_COMP, _("Unable to remove wire in unwireNodes()."));
            return -1;
        }

    }

    return 0;
}


static const char REWIRE_NODES[] =
    "SELECT srcFacadeInt,srcOut,destIn FROM SceneNodeEdge WHERE destFacadeInt=0";
static sqlite3_stmt* REWIRE_NODES_S;

static const char REWIRE_NODES_DEST[] =
    "SELECT srcFacadeInt,srcOut FROM SceneNodeEdge WHERE destIn=?1";
static sqlite3_stmt* REWIRE_NODES_DEST_S;

int
lsddb_rewireNodes ()
{
    sqlite3_reset (REWIRE_NODES_S);
    while (sqlite3_step (REWIRE_NODES_S) == SQLITE_ROW)
    {
        int destIn = sqlite3_column_int (REWIRE_NODES_S, 2);
        int srcOut = sqlite3_column_int (REWIRE_NODES_S, 1);
        int srcFacadeInt = sqlite3_column_int (REWIRE_NODES_S, 0);

        while (srcFacadeInt)
        {
            sqlite3_reset (REWIRE_NODES_DEST_S);
            sqlite3_bind_int (REWIRE_NODES_DEST_S, 1, srcOut);
            if (sqlite3_step (REWIRE_NODES_DEST_S) == SQLITE_ROW)
            {
                srcOut = sqlite3_column_int (REWIRE_NODES_DEST_S, 1);
                srcFacadeInt = sqlite3_column_int (REWIRE_NODES_DEST_S, 0);
            }
            else  /* No external facade input connection */
            {
                srcOut = -2;
                break;
            }
        }

        if (srcOut >= 0)
        {

            struct LSD_SceneNodeInput* dest = NULL;
            lsddb_traceInput (&dest, destIn, NULL, NULL);
            struct LSD_SceneNodeOutput* src = NULL;
            lsddb_traceOutput (&src, srcOut, NULL, NULL);

            if (dest && src)
                dest->connection = src;
            else if (dest)
            {
                dest->connection = NULL;
                doLog (ERROR, LOG_COMP, _("Problem while rewiring nodes."));
            }

        }
    }

    return 0;
}


static const char JSON_CLASS_LIBRARY[] =
    "SELECT id,name FROM SceneNodeClass";
static sqlite3_stmt* JSON_CLASS_LIBRARY_S;

int
lsddb_jsonClassLibrary (cJSON* target)
{
    if (!target)
    {
        doLog (ERROR, LOG_COMP, _("target may not be null in jsonClassLibrary()."));
        return -1;
    }

    cJSON* classArr = cJSON_CreateArray ();

    sqlite3_reset (JSON_CLASS_LIBRARY_S);

    while (sqlite3_step (JSON_CLASS_LIBRARY_S) == SQLITE_ROW)
    {
        int classId = sqlite3_column_int (JSON_CLASS_LIBRARY_S, 0);
        const char* className = (const char*)sqlite3_column_text (
            JSON_CLASS_LIBRARY_S,
            1);

        if (lsddb_checkClassEnabled (classId))
        {

            cJSON* classObj = cJSON_CreateObject ();

            cJSON_AddNumberToObject (classObj, "classId", classId);
            cJSON_AddStringToObject (classObj, "className", className);

            cJSON_AddItemToArray (classArr, classObj);

        }
    }

    cJSON_AddItemToObject (target, "classes", classArr);

    return 0;
}


static const char JSON_GET_FACADE_OUTS[] =
    "SELECT id,name FROM SceneNodeInstOutput WHERE instId=?1 AND facadeBool=1";
static sqlite3_stmt* JSON_GET_FACADE_OUTS_S;

int
lsddb_jsonGetFacadeOuts (int psId, cJSON* target)
{
    if (!target)
        return -1;

    cJSON* outArr = cJSON_CreateArray ();

    sqlite3_reset (JSON_GET_FACADE_OUTS_S);
    sqlite3_bind_int (JSON_GET_FACADE_OUTS_S, 1, psId);
    while (sqlite3_step (JSON_GET_FACADE_OUTS_S) == SQLITE_ROW)
    {
        int outId = sqlite3_column_int (JSON_GET_FACADE_OUTS_S, 0);
        const unsigned char* outName = sqlite3_column_text (
            JSON_GET_FACADE_OUTS_S,
            1);

        cJSON* outObj = cJSON_CreateObject ();
        cJSON_AddNumberToObject (outObj, "outId", outId);
        cJSON_AddStringToObject (outObj, "outName", (const char*)outName);
        cJSON_AddItemToArray (outArr, outObj);
    }

    cJSON_AddItemToObject (target, "facadeOuts", outArr);

    return 0;
}


static const char JSON_GET_FACADE_INS[] =
    "SELECT id,name FROM SceneNodeInstInput WHERE instId=?1 AND facadeBool=1";
static sqlite3_stmt* JSON_GET_FACADE_INS_S;

int
lsddb_jsonGetFacadeIns (int psId, cJSON* target)
{
    if (!target)
        return -1;

    cJSON* inArr = cJSON_CreateArray ();

    sqlite3_reset (JSON_GET_FACADE_INS_S);
    sqlite3_bind_int (JSON_GET_FACADE_INS_S, 1, psId);
    while (sqlite3_step (JSON_GET_FACADE_INS_S) == SQLITE_ROW)
    {
        int inId = sqlite3_column_int (JSON_GET_FACADE_INS_S, 0);
        const unsigned char* inName = sqlite3_column_text (
            JSON_GET_FACADE_INS_S,
            1);

        cJSON* inObj = cJSON_CreateObject ();
        cJSON_AddNumberToObject (inObj, "inId", inId);
        cJSON_AddStringToObject (inObj, "inName", (const char*)inName);
        cJSON_AddItemToArray (inArr, inObj);
    }

    cJSON_AddItemToObject (target, "facadeIns", inArr);

    return 0;
}


static const char JSON_PARTS[] =
    "SELECT id,name,patchSpaceId,imageUrl FROM SystemPartition";
static sqlite3_stmt* JSON_PARTS_S;

int
lsddb_jsonParts (cJSON* target)
{
    if (!target)
    {
        doLog (ERROR, LOG_COMP, _("target may not be null in jsonParts()."));
        return -1;
    }

    cJSON* partArr = cJSON_CreateArray ();

    sqlite3_reset (JSON_PARTS_S);

    while (sqlite3_step (JSON_PARTS_S) == SQLITE_ROW)
    {
        cJSON* partObj = cJSON_CreateObject ();

        int partId = sqlite3_column_int (JSON_PARTS_S, 0);
        const char* partName = (const char*)sqlite3_column_text (JSON_PARTS_S,
                                                                 1);
        int psId = sqlite3_column_int (JSON_PARTS_S, 2);
        const char* imageUrl = (const char*)sqlite3_column_text (JSON_PARTS_S,
                                                                 3);

        cJSON_AddNumberToObject (partObj, "partId", partId);
        cJSON_AddStringToObject (partObj, "partName", partName);
        cJSON_AddNumberToObject (partObj, "psId", psId);
        if (imageUrl)
            cJSON_AddStringToObject (partObj, "imageUrl", imageUrl);

        /* Add partition facade outs */
        /* lsddb_jsonGetFacadeOuts(psId,partObj); */

        cJSON_AddItemToArray (partArr, partObj);
    }

    cJSON_AddItemToObject (target, "partitions", partArr);

    return 0;
}


static const char JSON_INSERT_CLASS_OBJECT[] =
    "SELECT pluginId,classIdx FROM SceneNodeClass WHERE id=?1";
static sqlite3_stmt* JSON_INSERT_CLASS_OBJECT_S;

/* Inserts a 'class' object into a json object consisting of
 * classId, */
/* pluginId, and classIdx to allow a node in the client to
 * resolve its */
/* various class members implemented in static files */
int
lsddb_jsonInsertClassObject (cJSON* target, int classId)
{
    if (!target || target->type != cJSON_Object)
        return -1;

    sqlite3_reset (JSON_INSERT_CLASS_OBJECT_S);
    sqlite3_bind_int (JSON_INSERT_CLASS_OBJECT_S, 1, classId);

    if (sqlite3_step (JSON_INSERT_CLASS_OBJECT_S) == SQLITE_ROW)
    {
        int pluginId = sqlite3_column_int (JSON_INSERT_CLASS_OBJECT_S, 0);
        int classIdx = sqlite3_column_int (JSON_INSERT_CLASS_OBJECT_S, 1);

        cJSON* classObj = cJSON_CreateObject ();
        cJSON_AddNumberToObject (classObj, "classId", classId);
        cJSON_AddNumberToObject (classObj, "pluginId", pluginId);
        cJSON_AddNumberToObject (classObj, "classIdx", classIdx);

        cJSON_AddItemToObject (target, "classObj", classObj);

        return 0;
    }
    doLog (ERROR, LOG_COMP, _("Unable to resolve class from DB in jsonInsertClassObject()."));
    return -1;
}


static const char JSON_NODES[] =
    "SELECT id,posX,posY,classId,name,colourR,colourG,colourB FROM SceneNodeInst WHERE patchSpaceId=?1";
static sqlite3_stmt* JSON_NODES_S;
static const char JSON_NODES_INS[] =
    "SELECT id,typeId,name FROM SceneNodeInstInput WHERE instId=?1 AND facadeBool=0";
static sqlite3_stmt* JSON_NODES_INS_S;
static const char JSON_NODES_OUTS[] =
    "SELECT id,typeId,name FROM SceneNodeInstOutput WHERE instId=?1 AND facadeBool=0";
static sqlite3_stmt* JSON_NODES_OUTS_S;

static const char JSON_NODES_FACADES[] =
    "SELECT id,posX,posY,name,colourR,colourG,colourB FROM ScenePatchSpace WHERE parentPatchSpace=?1";
static sqlite3_stmt* JSON_NODES_FACADES_S;
static const char JSON_NODES_FACADES_INS[] =
    "SELECT id,typeId,name FROM SceneNodeInstInput WHERE instId=?1 AND facadeBool=1";
static sqlite3_stmt* JSON_NODES_FACADES_INS_S;
static const char JSON_NODES_FACADES_OUTS[] =
    "SELECT id,typeId,name FROM SceneNodeInstOutput WHERE instId=?1 AND facadeBool=1";
static sqlite3_stmt* JSON_NODES_FACADES_OUTS_S;

int
lsddb_jsonNodes (int patchSpaceId, cJSON* resp)
{
    sqlite3_reset (JSON_NODES_S);
    sqlite3_bind_int (JSON_NODES_S, 1, patchSpaceId);

    cJSON* nodeArr = cJSON_CreateArray ();

    while (sqlite3_step (JSON_NODES_S) == SQLITE_ROW)
    {
        cJSON* nodeObj = cJSON_CreateObject ();

        int nodeId = sqlite3_column_int (JSON_NODES_S, 0);
        int posX = sqlite3_column_int (JSON_NODES_S, 1);
        int posY = sqlite3_column_int (JSON_NODES_S, 2);
        const unsigned char* name = sqlite3_column_text (JSON_NODES_S, 4);
        int classId = sqlite3_column_int (JSON_NODES_S, 3);

        cJSON_AddNumberToObject (nodeObj, "nodeId", nodeId);
        cJSON_AddNumberToObject (nodeObj, "x", posX);
        cJSON_AddNumberToObject (nodeObj, "y", posY);
        cJSON_AddStringToObject (nodeObj, "name", (const char*)name);

        /* Colour Object */
        cJSON* colourObj = cJSON_CreateObject ();
        cJSON_AddNumberToObject (colourObj, "r",
                                 sqlite3_column_double (JSON_NODES_S, 5));
        cJSON_AddNumberToObject (colourObj, "g",
                                 sqlite3_column_double (JSON_NODES_S, 6));
        cJSON_AddNumberToObject (colourObj, "b",
                                 sqlite3_column_double (JSON_NODES_S, 7));
        cJSON_AddItemToObject (nodeObj, "colour", colourObj);

        if (lsddb_checkClassEnabled (classId))
            cJSON_AddTrueToObject (nodeObj, "enabled");
        else
            cJSON_AddFalseToObject (nodeObj, "enabled");
        lsddb_jsonInsertClassObject (nodeObj, classId);

        /* Get node's ins */
        sqlite3_reset (JSON_NODES_INS_S);
        sqlite3_bind_int (JSON_NODES_INS_S, 1, nodeId);

        cJSON* nodeInArr = cJSON_CreateArray ();
        while (sqlite3_step (JSON_NODES_INS_S) == SQLITE_ROW)
        {
            cJSON* inObj = cJSON_CreateObject ();

            int inId = sqlite3_column_int (JSON_NODES_INS_S, 0);
            int typeId = sqlite3_column_int (JSON_NODES_INS_S, 1);
            const unsigned char* name = sqlite3_column_text (JSON_NODES_INS_S,
                                                             2);

            cJSON_AddNumberToObject (inObj, "inId", inId);
            cJSON_AddNumberToObject (inObj, "typeId", typeId);
            cJSON_AddStringToObject (inObj, "name", (const char*)name);

            cJSON_AddItemToArray (nodeInArr, inObj);
        }

        cJSON_AddItemToObject (nodeObj, "nodeIns", nodeInArr);

        /* Get node's outs */
        sqlite3_reset (JSON_NODES_OUTS_S);
        sqlite3_bind_int (JSON_NODES_OUTS_S, 1, nodeId);

        cJSON* nodeOutArr = cJSON_CreateArray ();
        while (sqlite3_step (JSON_NODES_OUTS_S) == SQLITE_ROW)
        {
            cJSON* outObj = cJSON_CreateObject ();

            int outId = sqlite3_column_int (JSON_NODES_OUTS_S, 0);
            int typeId = sqlite3_column_int (JSON_NODES_OUTS_S, 1);
            const unsigned char* name = sqlite3_column_text (JSON_NODES_OUTS_S,
                                                             2);

            cJSON_AddNumberToObject (outObj, "outId", outId);
            cJSON_AddNumberToObject (outObj, "typeId", typeId);
            cJSON_AddStringToObject (outObj, "name", (const char*)name);

            cJSON_AddItemToArray (nodeOutArr, outObj);
        }

        cJSON_AddItemToObject (nodeObj, "nodeOuts", nodeOutArr);

        /* Add node to array */
        cJSON_AddItemToArray (nodeArr, nodeObj);
    }

    /* Facades */

    sqlite3_reset (JSON_NODES_FACADES_S);
    sqlite3_bind_int (JSON_NODES_FACADES_S, 1, patchSpaceId);

    while (sqlite3_step (JSON_NODES_FACADES_S) == SQLITE_ROW)
    {
        cJSON* nodeObj = cJSON_CreateObject ();

        int nodeId = sqlite3_column_int (JSON_NODES_FACADES_S, 0);
        const unsigned char* psName = sqlite3_column_text (JSON_NODES_FACADES_S,
                                                           3);
        int posX = sqlite3_column_int (JSON_NODES_FACADES_S, 1);
        int posY = sqlite3_column_int (JSON_NODES_FACADES_S, 2);

        cJSON_AddNumberToObject (nodeObj, "facadeId", nodeId);
        cJSON_AddStringToObject (nodeObj, "name", (const char*)psName);
        cJSON_AddNumberToObject (nodeObj, "x", posX);
        cJSON_AddNumberToObject (nodeObj, "y", posY);

        /* Colour Object */
        cJSON* colourObj = cJSON_CreateObject ();
        cJSON_AddNumberToObject (colourObj, "r",
                                 sqlite3_column_double (JSON_NODES_FACADES_S, 4));
        cJSON_AddNumberToObject (colourObj, "g",
                                 sqlite3_column_double (JSON_NODES_FACADES_S, 5));
        cJSON_AddNumberToObject (colourObj, "b",
                                 sqlite3_column_double (JSON_NODES_FACADES_S, 6));
        cJSON_AddItemToObject (nodeObj, "colour", colourObj);

        /* Get facade's ins */
        sqlite3_reset (JSON_NODES_FACADES_INS_S);
        sqlite3_bind_int (JSON_NODES_FACADES_INS_S, 1, nodeId);

        cJSON* nodeInArr = cJSON_CreateArray ();
        while (sqlite3_step (JSON_NODES_FACADES_INS_S) == SQLITE_ROW)
        {
            cJSON* inObj = cJSON_CreateObject ();

            int inId = sqlite3_column_int (JSON_NODES_FACADES_INS_S, 0);
            int typeId = sqlite3_column_int (JSON_NODES_FACADES_INS_S, 1);
            const unsigned char* name = sqlite3_column_text (
                JSON_NODES_FACADES_INS_S,
                2);

            cJSON_AddNumberToObject (inObj, "inId", inId);
            cJSON_AddNumberToObject (inObj, "typeId", typeId);
            cJSON_AddStringToObject (inObj, "name", (const char*)name);

            cJSON_AddItemToArray (nodeInArr, inObj);
        }

        cJSON_AddItemToObject (nodeObj, "facadeIns", nodeInArr);

        /* Get facade's outs */
        sqlite3_reset (JSON_NODES_FACADES_OUTS_S);
        sqlite3_bind_int (JSON_NODES_FACADES_OUTS_S, 1, nodeId);

        cJSON* nodeOutArr = cJSON_CreateArray ();
        while (sqlite3_step (JSON_NODES_FACADES_OUTS_S) == SQLITE_ROW)
        {
            cJSON* outObj = cJSON_CreateObject ();

            int outId = sqlite3_column_int (JSON_NODES_FACADES_OUTS_S, 0);
            int typeId = sqlite3_column_int (JSON_NODES_FACADES_OUTS_S, 1);
            const unsigned char* name = sqlite3_column_text (
                JSON_NODES_FACADES_OUTS_S,
                2);

            cJSON_AddNumberToObject (outObj, "outId", outId);
            cJSON_AddNumberToObject (outObj, "typeId", typeId);
            cJSON_AddStringToObject (outObj, "name", (const char*)name);

            cJSON_AddItemToArray (nodeOutArr, outObj);
        }

        cJSON_AddItemToObject (nodeObj, "facadeOuts", nodeOutArr);

        /* Add node to array */
        cJSON_AddItemToArray (nodeArr, nodeObj);
    }

    /* cJSON_AddNumberToObject(resp,"psId",patchSpaceId); */
    cJSON_AddItemToObject (resp, "nodes", nodeArr);

    return 0;
}


/* Return a single facade for editing purposes */
static const char JSON_GET_FACADE[] =
    "SELECT name FROM ScenePatchSpace WHERE id=?1";
static sqlite3_stmt* JSON_GET_FACADE_S;

int
lsddb_jsonFacade (int psId, cJSON* resp)
{
    sqlite3_reset (JSON_GET_FACADE_S);
    sqlite3_bind_int (JSON_GET_FACADE_S, 1, psId);
    if (sqlite3_step (JSON_GET_FACADE_S) == SQLITE_ROW)
    {
        cJSON* facObj = cJSON_CreateObject ();
        cJSON_AddNumberToObject (facObj, "psId", psId);

        const unsigned char* name = sqlite3_column_text (JSON_GET_FACADE_S, 0);
        cJSON_AddStringToObject (facObj, "name", (const char*)name);

        lsddb_jsonGetFacadeIns (psId, facObj);

        lsddb_jsonGetFacadeOuts (psId, facObj);

        cJSON_AddItemToObject (resp, "facade", facObj);
    }
    else
        return -1;

    return 0;
}


static const char JSON_WIRES[] =
    "SELECT id,srcOut,destIn,srcFacadeInt,destFacadeInt FROM SceneNodeEdge WHERE patchSpaceId=?1";
static sqlite3_stmt* JSON_WIRES_S;

int
lsddb_jsonWires (int patchSpaceId, cJSON* resp)
{
    sqlite3_reset (JSON_WIRES_S);
    sqlite3_bind_int (JSON_WIRES_S, 1, patchSpaceId);

    cJSON* wireArr = cJSON_CreateArray ();

    while (sqlite3_step (JSON_WIRES_S) == SQLITE_ROW)
    {
        cJSON* wireObj = cJSON_CreateObject ();

        int wireId = sqlite3_column_int (JSON_WIRES_S, 0);
        int wireLeft = sqlite3_column_int (JSON_WIRES_S, 1);
        int wireRight = sqlite3_column_int (JSON_WIRES_S, 2);
        int wireLeftInt = sqlite3_column_int (JSON_WIRES_S, 3);
        int wireRightInt = sqlite3_column_int (JSON_WIRES_S, 4);

        cJSON_AddNumberToObject (wireObj, "wireId", wireId);
        cJSON_AddNumberToObject (wireObj, "wireLeftInt", wireLeftInt);
        cJSON_AddNumberToObject (wireObj, "wireLeft", wireLeft);
        cJSON_AddNumberToObject (wireObj, "wireRightInt", wireRightInt);
        cJSON_AddNumberToObject (wireObj, "wireRight", wireRight);

        cJSON_AddItemToArray (wireArr, wireObj);
    }

    /* cJSON_AddNumberToObject(resp,"psId",patchSpaceId); */
    cJSON_AddItemToObject (resp, "wireArr", wireArr);

    return 0;
}


static const char JSON_PATCH_SPACE[] =
    "SELECT name,panX,panY,scale FROM ScenePatchSpace WHERE id=?1";
static sqlite3_stmt* JSON_PATCH_SPACE_S;

int
lsddb_jsonPatchSpace (int patchSpaceId, cJSON* resp)
{
    sqlite3_reset (JSON_PATCH_SPACE_S);
    sqlite3_bind_int (JSON_PATCH_SPACE_S, 1, patchSpaceId);

    if (sqlite3_step (JSON_PATCH_SPACE_S) == SQLITE_ROW)
    {
        const unsigned char* psName = sqlite3_column_text (JSON_PATCH_SPACE_S,
                                                           0);
        int panX = sqlite3_column_int (JSON_PATCH_SPACE_S, 1);
        int panY = sqlite3_column_int (JSON_PATCH_SPACE_S, 2);
        double scale = sqlite3_column_double (JSON_PATCH_SPACE_S, 3);

        cJSON_AddNumberToObject (resp, "psId", patchSpaceId);
        cJSON_AddStringToObject (resp, "name", (const char*)psName);
        cJSON_AddNumberToObject (resp, "x", panX);
        cJSON_AddNumberToObject (resp, "y", panY);
        cJSON_AddNumberToObject (resp, "scale", scale);

        lsddb_jsonNodes (patchSpaceId, resp);
        lsddb_jsonWires (patchSpaceId, resp);
        lsddb_jsonGetFacadeIns (patchSpaceId, resp);
        lsddb_jsonGetFacadeOuts (patchSpaceId, resp);
    }
    else
    {
        cJSON_AddStringToObject (resp, "error", _("Patch Space Non-existent"));
        return -1;
    }

    return 0;
}


static const char RESOLVE_CLASS_FROM_ID[] =
    "SELECT arrayIdx FROM SceneNodeClass WHERE id=?1";
static sqlite3_stmt* RESOLVE_CLASS_FROM_ID_S;

int
lsddb_resolveClassFromId (struct LSD_SceneNodeClass** ptrToBind, int classId)
{
    sqlite3_reset (RESOLVE_CLASS_FROM_ID_S);
    sqlite3_bind_int (RESOLVE_CLASS_FROM_ID_S, 1, classId);

    struct LSD_SceneNodeClass* pickedClass;

    if (sqlite3_step (RESOLVE_CLASS_FROM_ID_S) == SQLITE_ROW)
    {
        int arrIdx = sqlite3_column_int (RESOLVE_CLASS_FROM_ID_S, 0);

        if (pickIdx (getArr_lsdNodeClassArr (), (void**)&pickedClass,
                     arrIdx) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to pick class from array in resolveClassFromId()."));
            return -1;
        }
    }
    else
    {
        doLog (ERROR, LOG_COMP, _("Class could not be resolved or its plugin is disabled in resolveClassFromId()."));
        return -1;
    }

    if (ptrToBind)
        *ptrToBind = pickedClass;

    return 0;
}


static const char RESOLVE_INST_FROM_ID[] =
    "SELECT arrIdx FROM SceneNodeInst WHERE id=?1";
static sqlite3_stmt* RESOLVE_INST_FROM_ID_S;

int
lsddb_resolveInstFromId (struct LSD_SceneNodeInst const** target,
                         int nodeId,
                         void** dataBind)
{
    if (!target)
        return -1;

    sqlite3_reset (RESOLVE_INST_FROM_ID_S);
    sqlite3_bind_int (RESOLVE_INST_FROM_ID_S, 1, nodeId);

    struct LSD_SceneNodeInst* pickedInst;

    if (sqlite3_step (RESOLVE_INST_FROM_ID_S) == SQLITE_ROW)
    {
        int arrIdx = sqlite3_column_int (RESOLVE_INST_FROM_ID_S, 0);

        if (pickIdx (getArr_lsdNodeInstArr (), (void**)&pickedInst, arrIdx) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to pick inst from array in resolveInstFromId()."));
            return -1;
        }
    }
    else
    {
        doLog (ERROR, LOG_COMP, _("Inst could not be resolved in DB in resolveInstFromId()."));
        return -1;
    }

    *target = pickedInst;

    if (dataBind)
        *dataBind = pickedInst->data;

    return 0;
}


static const char RESOLVE_INST_FROM_IN_ID[] =
    "SELECT SceneNodeInst.arrIdx FROM SceneNodeInst,SceneNodeInstInput WHERE "
    "SceneNodeInstInput.instId=SceneNodeInst.id AND SceneNodeInstInput.facadeBool=0 "
    "AND SceneNodeInstInput.id=?1";
static sqlite3_stmt* RESOLVE_INST_FROM_IN_ID_S;

int
lsddb_resolveInstFromInId (struct LSD_SceneNodeInst const** target, int inId)
{
    if (!target)
        return -1;

    sqlite3_reset (RESOLVE_INST_FROM_IN_ID_S);
    sqlite3_bind_int (RESOLVE_INST_FROM_IN_ID_S, 1, inId);

    struct LSD_SceneNodeInst* pickedInst;

    if (sqlite3_step (RESOLVE_INST_FROM_IN_ID_S) == SQLITE_ROW)
    {
        int arrIdx = sqlite3_column_int (RESOLVE_INST_FROM_IN_ID_S, 0);

        if (pickIdx (getArr_lsdNodeInstArr (), (void**)&pickedInst, arrIdx) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to pick inst from array in resolveInstFromInId()."));
            return -1;
        }
    }
    else
    {
        doLog (ERROR, LOG_COMP, _("Inst could not be resolved in DB in resolveInstFromInId()."));
        return -1;
    }

    *target = pickedInst;

    return 0;
}


static const char RESOLVE_INST_FROM_OUT_ID[] =
    "SELECT SceneNodeInst.arrIdx FROM SceneNodeInst,SceneNodeInstOutput WHERE "
    "SceneNodeInstOutput.instId=SceneNodeInst.id AND SceneNodeInstOutput.facadeBool=0 "
    "AND SceneNodeInstOutput.id=?1";
static sqlite3_stmt* RESOLVE_INST_FROM_OUT_ID_S;

int
lsddb_resolveInstFromOutId (struct LSD_SceneNodeInst const** target, int outId)
{
    if (!target)
        return -1;

    sqlite3_reset (RESOLVE_INST_FROM_OUT_ID_S);
    sqlite3_bind_int (RESOLVE_INST_FROM_OUT_ID_S, 1, outId);

    struct LSD_SceneNodeInst* pickedInst;

    if (sqlite3_step (RESOLVE_INST_FROM_OUT_ID_S) == SQLITE_ROW)
    {
        int arrIdx = sqlite3_column_int (RESOLVE_INST_FROM_OUT_ID_S, 0);

        if (pickIdx (getArr_lsdNodeInstArr (), (void**)&pickedInst, arrIdx) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to pick inst from array in resolveInstFromOutId()."));
            return -1;
        }
    }
    else
    {
        doLog (ERROR, LOG_COMP, _("Inst %d could not be resolved in DB in resolveInstFromOutId()\n%s."),
            outId, sqlite3_errmsg (memdb));
        return -1;
    }

    *target = pickedInst;

    return 0;
}


int
lsddb_resolveInputFromId (struct LSD_SceneNodeInput** inBind, int inId)
{
    if (!inBind)
        return -1;

    sqlite3_reset (REMOVE_NODE_INST_INPUT_ARRIDX_S);
    sqlite3_bind_int (REMOVE_NODE_INST_INPUT_ARRIDX_S, 1, inId);
    if (sqlite3_step (REMOVE_NODE_INST_INPUT_ARRIDX_S) == SQLITE_ROW)
    {
        int arrIdx = sqlite3_column_int (REMOVE_NODE_INST_INPUT_ARRIDX_S, 0);
        if (arrIdx < 0)
            return -1;

        if (pickIdx (getArr_lsdNodeInputArr (), (void**)inBind, arrIdx) < 0)
        {
            doLog (ERROR, LOG_COMP, _("Unable to pick input from array in resolveInputFromId()."));
            return -1;
        }
        return 0;
    }

    return -1;
}


/* Widget operations below */
//static const char 


/* Channel patch operations below */

static const char GET_PATCH_CHANNELS_PARTS[] =
    "SELECT id,name FROM SystemPartition";
static sqlite3_stmt* GET_PATCH_CHANNELS_PARTS_S;

static const char GET_PATCH_CHANNELS_CHANS[] =
    "SELECT SystemChannel.id,SystemChannel.name,SystemChannel.single,OlaAddress.olaUnivId,OlaAddress.olaLightAddr,OlaAddress.sixteenBit,SystemChannel.gAddrId,SystemChannel.bAddrId FROM SystemChannel,OlaAddress WHERE OlaAddress.id=SystemChannel.rAddrId AND SystemChannel.partitionId=?1";
static sqlite3_stmt* GET_PATCH_CHANNELS_CHANS_S;

static const char GET_PATCH_CHANNELS_ADDITIONAL[] =
    "SELECT olaUnivId,olaLightAddr FROM OlaAddress WHERE id=?1";
static sqlite3_stmt* GET_PATCH_CHANNELS_ADDITIONAL_S;

int
lsddb_getPatchChannels (cJSON* target)
{
    if (!target)
        return -1;

    /* Begin partition iteration */
    cJSON* partArr = cJSON_CreateArray ();

    sqlite3_reset (GET_PATCH_CHANNELS_PARTS_S);

    while (sqlite3_step (GET_PATCH_CHANNELS_PARTS_S) == SQLITE_ROW)
    {
        int partId = sqlite3_column_int (GET_PATCH_CHANNELS_PARTS_S, 0);
        const unsigned char* partName = sqlite3_column_text (
            GET_PATCH_CHANNELS_PARTS_S,
            1);

        cJSON* partObj = cJSON_CreateObject ();
        cJSON_AddNumberToObject (partObj, "partId", partId);
        cJSON_AddStringToObject (partObj, "partName", (const char*)partName);

        /* Construct channel array for this partition */
        cJSON* chanArr = cJSON_CreateArray ();

        sqlite3_reset (GET_PATCH_CHANNELS_CHANS_S);
        sqlite3_bind_int (GET_PATCH_CHANNELS_CHANS_S, 1, partId);

        while (sqlite3_step (GET_PATCH_CHANNELS_CHANS_S) == SQLITE_ROW)
        {
            int chanId = sqlite3_column_int (GET_PATCH_CHANNELS_CHANS_S, 0);
            const unsigned char* chanName = sqlite3_column_text (
                GET_PATCH_CHANNELS_CHANS_S,
                1);
            int single = sqlite3_column_int (GET_PATCH_CHANNELS_CHANS_S, 2);
            int rUnivId = sqlite3_column_int (GET_PATCH_CHANNELS_CHANS_S, 3);
            int rLightAddr = sqlite3_column_int (GET_PATCH_CHANNELS_CHANS_S, 4);
            int sixteenBit = sqlite3_column_int (GET_PATCH_CHANNELS_CHANS_S, 5);

            cJSON* chanObj = cJSON_CreateObject ();
            cJSON_AddNumberToObject (chanObj, "chanId", chanId);
            cJSON_AddStringToObject (chanObj, "chanName", (const char*)chanName);
            cJSON_AddNumberToObject (chanObj, "single", single);
            cJSON_AddNumberToObject (chanObj, "sixteenBit", sixteenBit);

            cJSON* rObj = cJSON_CreateObject ();
            cJSON_AddNumberToObject (rObj, "univId", rUnivId);
            cJSON_AddNumberToObject (rObj, "lightAddr", rLightAddr);
            cJSON_AddItemToObject (chanObj, "redAddr", rObj);

            /* If RGB channel, get green and blue address
             * data */
            if (!single)
            {
                int gAddrId = sqlite3_column_int (GET_PATCH_CHANNELS_CHANS_S, 6);
                int bAddrId = sqlite3_column_int (GET_PATCH_CHANNELS_CHANS_S, 7);

                /* Green */
                sqlite3_reset (GET_PATCH_CHANNELS_ADDITIONAL_S);
                sqlite3_bind_int (GET_PATCH_CHANNELS_ADDITIONAL_S, 1, gAddrId);
                if (sqlite3_step (GET_PATCH_CHANNELS_ADDITIONAL_S) ==
                    SQLITE_ROW)
                {
                    int gUnivId = sqlite3_column_int (
                        GET_PATCH_CHANNELS_ADDITIONAL_S,
                        0);
                    int gLightAddr = sqlite3_column_int (
                        GET_PATCH_CHANNELS_ADDITIONAL_S,
                        1);

                    cJSON* gObj = cJSON_CreateObject ();
                    cJSON_AddNumberToObject (gObj, "univId", gUnivId);
                    cJSON_AddNumberToObject (gObj, "lightAddr", gLightAddr);

                    cJSON_AddItemToObject (chanObj, "greenAddr", gObj);
                }

                /* Blue */
                sqlite3_reset (GET_PATCH_CHANNELS_ADDITIONAL_S);
                sqlite3_bind_int (GET_PATCH_CHANNELS_ADDITIONAL_S, 1, bAddrId);
                if (sqlite3_step (GET_PATCH_CHANNELS_ADDITIONAL_S) ==
                    SQLITE_ROW)
                {
                    int bUnivId = sqlite3_column_int (
                        GET_PATCH_CHANNELS_ADDITIONAL_S,
                        0);
                    int bLightAddr = sqlite3_column_int (
                        GET_PATCH_CHANNELS_ADDITIONAL_S,
                        1);

                    cJSON* bObj = cJSON_CreateObject ();
                    cJSON_AddNumberToObject (bObj, "univId", bUnivId);
                    cJSON_AddNumberToObject (bObj, "lightAddr", bLightAddr);

                    cJSON_AddItemToObject (chanObj, "blueAddr", bObj);
                }
            }

            cJSON_AddItemToArray (chanArr, chanObj);
        }
        cJSON_AddItemToObject (partObj, "channels", chanArr);
        cJSON_AddItemToArray (partArr, partObj);
    }

    cJSON_AddItemToObject (target, "partitions", partArr);

    return 0;
}


static const char ADD_PATCH_CHANNEL[] =
    "INSERT INTO SystemChannel (name,partitionId,single,rAddrId,gAddrId,bAddrId,facadeOutId) VALUES (?1,?2,?3,?4,?5,?6,?7)";
static sqlite3_stmt* ADD_PATCH_CHANNEL_S;

static const char ADD_PATCH_CHANNEL_ADDR[] =
    "INSERT INTO OlaAddress (olaUnivId,olaLightAddr,sixteenBit) VALUES (?1,?2,?3)";
static sqlite3_stmt* ADD_PATCH_CHANNEL_ADDR_S;

static const char ADD_PATCH_CHANNEL_FACADE_OUT[] =
    "INSERT INTO SceneNodeInstOutput (instId,typeId,facadeBool,name,bfFuncIdx,bpFuncIdx) VALUES (?1,-1,1,?2,-1,-1)";
static sqlite3_stmt* ADD_PATCH_CHANNEL_FACADE_OUT_S;

int
lsddb_addPatchChannelAddr (int* addrIdBind, cJSON* addrObj, int sixteenBit)
{
    if (!addrObj)
        return -1;

    cJSON* univId = cJSON_GetObjectItem (addrObj, "univId");
    if (!univId || univId->type != cJSON_Number)
        return -1;

    cJSON* lightAddr = cJSON_GetObjectItem (addrObj, "lightAddr");
    if (!lightAddr || lightAddr->type != cJSON_Number)
        return -1;

    sqlite3_reset (ADD_PATCH_CHANNEL_ADDR_S);
    sqlite3_bind_int (ADD_PATCH_CHANNEL_ADDR_S, 1, univId->valueint);
    sqlite3_bind_int (ADD_PATCH_CHANNEL_ADDR_S, 2, lightAddr->valueint);
    sqlite3_bind_int (ADD_PATCH_CHANNEL_ADDR_S, 3, sixteenBit);

    if (sqlite3_step (ADD_PATCH_CHANNEL_ADDR_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Error while adding address into DB\n Details: %s"), 
                                  sqlite3_errmsg (memdb));
        return -1;
    }

    if (addrIdBind)
        *addrIdBind = sqlite3_last_insert_rowid (memdb);

    return 0;
}


int
lsddb_addPatchChannel (int partId, cJSON* opts)
{
    if (!opts)
        return -1;

    cJSON* name = cJSON_GetObjectItem (opts, "name");
    if (!name || name->type != cJSON_String)
        return -1;

    cJSON* single = cJSON_GetObjectItem (opts, "single");
    if (!single || single->type != cJSON_Number)
        return -1;

    cJSON* sixteenBit = cJSON_GetObjectItem (opts, "sixteenBit");
    if (!sixteenBit || sixteenBit->type != cJSON_Number)
        return -1;

    cJSON* rAddr = cJSON_GetObjectItem (opts, "redAddr");
    if (!rAddr || rAddr->type != cJSON_Object)
        return -1;

    cJSON* gAddr = NULL;
    cJSON* bAddr = NULL;
    if (!single->valueint)
    {
        gAddr = cJSON_GetObjectItem (opts, "greenAddr");
        if (!gAddr || gAddr->type != cJSON_Object)
            return -1;

        bAddr = cJSON_GetObjectItem (opts, "blueAddr");
        if (!bAddr || bAddr->type != cJSON_Object)
            return -1;
    }

    sqlite3_reset (ADD_PATCH_CHANNEL_S);
    sqlite3_bind_text (ADD_PATCH_CHANNEL_S, 1, name->valuestring, -1, NULL);
    sqlite3_bind_int (ADD_PATCH_CHANNEL_S, 2, partId);
    sqlite3_bind_int (ADD_PATCH_CHANNEL_S, 3, single->valueint);

    int rAddrId;
    lsddb_addPatchChannelAddr (&rAddrId, rAddr, sixteenBit->valueint);
    sqlite3_bind_int (ADD_PATCH_CHANNEL_S, 4, rAddrId);

    if (!single->valueint)
    {
        int gAddrId;
        lsddb_addPatchChannelAddr (&gAddrId, gAddr, sixteenBit->valueint);
        sqlite3_bind_int (ADD_PATCH_CHANNEL_S, 5, gAddrId);

        int bAddrId;
        lsddb_addPatchChannelAddr (&bAddrId, bAddr, sixteenBit->valueint);
        sqlite3_bind_int (ADD_PATCH_CHANNEL_S, 6, bAddrId);
    }

    /* Add facade Out to partition's patchSpace's facade */
    int psId;
    sqlite3_reset (GET_PARTITON_PATCHSPACE_S);
    sqlite3_bind_int (GET_PARTITON_PATCHSPACE_S, 1, partId);
    if (sqlite3_step (GET_PARTITON_PATCHSPACE_S) == SQLITE_ROW)
        psId = sqlite3_column_int (GET_PARTITON_PATCHSPACE_S, 0);
    else
    {
        doLog (ERROR, LOG_COMP, _("Unable to resolve partition's patchSpace in addPatchChannel()."));
        return -1;
    }

    sqlite3_reset (ADD_PATCH_CHANNEL_FACADE_OUT_S);
    sqlite3_bind_int (ADD_PATCH_CHANNEL_FACADE_OUT_S, 1, psId);
    sqlite3_bind_text (ADD_PATCH_CHANNEL_FACADE_OUT_S,
                       2,
                       name->valuestring,
                       -1,
                       NULL);
    if (sqlite3_step (ADD_PATCH_CHANNEL_FACADE_OUT_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Unable to add facade out in addPatchChannel()\nDetails: %s"),
               sqlite3_errmsg (memdb));
        return -1;
    }

    int facadeOut = sqlite3_last_insert_rowid (memdb);
    /* printf("Added Facade Out: %d\n",facadeOut); */

    sqlite3_bind_int (ADD_PATCH_CHANNEL_S, 7, facadeOut);

    if (sqlite3_step (ADD_PATCH_CHANNEL_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Error while inserting channel into DB\nDetails: %s"),
                                  sqlite3_errmsg (memdb));
        return -1;
    }

    return 0;
}


/* Helper statement to get addressIds from channel for
 * updating/deleting */
static const char CHANNEL_GET_ADDRS[] =
    "SELECT single,rAddrId,gAddrId,bAddrId,facadeOutId FROM SystemChannel WHERE id=?1";
static sqlite3_stmt* CHANNEL_GET_ADDRS_S;

static const char UPDATE_PATCH_CHANNEL[] =
    "UPDATE SystemChannel SET name=?2,single=?3,rAddrId=?4,gAddrId=?5,bAddrId=?6 WHERE id=?1";
static sqlite3_stmt* UPDATE_PATCH_CHANNEL_S;

/* Forward declaration of delete addr for use in update */
static const char DELETE_PATCH_CHANNEL_ADDR[] =
    "DELETE FROM OlaAddress WHERE id=?1";
static sqlite3_stmt* DELETE_PATCH_CHANNEL_ADDR_S;

/* Helper for deleting addresses */
int
lsddb_deletePatchChannelAddr (int addrId)
{
    sqlite3_reset (DELETE_PATCH_CHANNEL_ADDR_S);
    sqlite3_bind_int (DELETE_PATCH_CHANNEL_ADDR_S, 1, addrId);
    if (sqlite3_step (DELETE_PATCH_CHANNEL_ADDR_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Unable to remove address on behalf of deletePatchChannel()."));
        return -1;
    }
    return 0;
}


/* Updating statement to handle name changes for facadeOut */
static const char UPDATE_PATCH_CHANNEL_FACADE_OUT[] =
    "UPDATE SceneNodeInstOutput SET name=?2 WHERE id=?1";
static sqlite3_stmt* UPDATE_PATCH_CHANNEL_FACADE_OUT_S;

int
lsddb_updatePatchChannel (int chanId, cJSON* opts)
{
    if (!opts)
        return -1;

    /* Check to see if all fields are present to ensure
     * atomicity */

    cJSON* name = cJSON_GetObjectItem (opts, "name");
    if (!name || name->type != cJSON_String)
        return -1;

    cJSON* single = cJSON_GetObjectItem (opts, "single");
    if (!single || single->type != cJSON_Number)
        return -1;

    cJSON* sixteenBit = cJSON_GetObjectItem (opts, "sixteenBit");
    if (!sixteenBit || sixteenBit->type != cJSON_Number)
        return -1;

    cJSON* rAddr = cJSON_GetObjectItem (opts, "redAddr");
    if (!rAddr || rAddr->type != cJSON_Object)
        return -1;

    cJSON* gAddr = NULL;
    cJSON* bAddr = NULL;
    if (!single->valueint)
    {
        gAddr = cJSON_GetObjectItem (opts, "greenAddr");
        if (!gAddr || gAddr->type != cJSON_Object)
            return -1;

        bAddr = cJSON_GetObjectItem (opts, "blueAddr");
        if (!bAddr || bAddr->type != cJSON_Object)
            return -1;
    }

    /* Delete old addresses */
    sqlite3_reset (CHANNEL_GET_ADDRS_S);
    sqlite3_bind_int (CHANNEL_GET_ADDRS_S, 1, chanId);

    if (sqlite3_step (CHANNEL_GET_ADDRS_S) == SQLITE_ROW)
    {
        int singleOld = sqlite3_column_int (CHANNEL_GET_ADDRS_S, 0);
        int rAddrIdOld = sqlite3_column_int (CHANNEL_GET_ADDRS_S, 1);
        lsddb_deletePatchChannelAddr (rAddrIdOld);

        if (!singleOld)
        {
            int gAddrIdOld = sqlite3_column_int (CHANNEL_GET_ADDRS_S, 2);
            lsddb_deletePatchChannelAddr (gAddrIdOld);
            int bAddrIdOld = sqlite3_column_int (CHANNEL_GET_ADDRS_S, 3);
            lsddb_deletePatchChannelAddr (bAddrIdOld);
        }

        /* Update facadeOut name */
        int facadeOutId = sqlite3_column_int (CHANNEL_GET_ADDRS_S, 4);
        sqlite3_reset (UPDATE_PATCH_CHANNEL_FACADE_OUT_S);
        sqlite3_bind_int (UPDATE_PATCH_CHANNEL_FACADE_OUT_S, 1, facadeOutId);
        sqlite3_bind_text (UPDATE_PATCH_CHANNEL_FACADE_OUT_S,
                           2,
                           name->valuestring,
                           -1,
                           NULL);
        if (sqlite3_step (UPDATE_PATCH_CHANNEL_FACADE_OUT_S) != SQLITE_DONE)
            doLog (ERROR, LOG_COMP, _("Unable to update facadeOut's name in updatePatchChannel()."));
    }
    else
    {
        doLog (ERROR, LOG_COMP, _("Unable to discover addresses in updatePatchChannel()."));
        return -1;
    }

    /* Insert new Addresses and record them in channel */
    sqlite3_reset (UPDATE_PATCH_CHANNEL_S);
    sqlite3_bind_int (UPDATE_PATCH_CHANNEL_S, 1, chanId);
    sqlite3_bind_text (UPDATE_PATCH_CHANNEL_S, 2, name->valuestring, -1, NULL);
    sqlite3_bind_int (UPDATE_PATCH_CHANNEL_S, 3, single->valueint);

    int rAddrId;
    lsddb_addPatchChannelAddr (&rAddrId, rAddr, sixteenBit->valueint);
    sqlite3_bind_int (UPDATE_PATCH_CHANNEL_S, 4, rAddrId);

    if (!single->valueint)
    {
        int gAddrId;
        lsddb_addPatchChannelAddr (&gAddrId, gAddr, sixteenBit->valueint);
        sqlite3_bind_int (UPDATE_PATCH_CHANNEL_S, 5, gAddrId);

        int bAddrId;
        lsddb_addPatchChannelAddr (&bAddrId, bAddr, sixteenBit->valueint);
        sqlite3_bind_int (UPDATE_PATCH_CHANNEL_S, 6, bAddrId);
    }

    if (sqlite3_step (UPDATE_PATCH_CHANNEL_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Error while updating channel on DB."));
        return -1;
    }

    return 0;
}


static const char DELETE_PATCH_CHANNEL[] =
    "DELETE FROM SystemChannel WHERE id=?1";
static sqlite3_stmt* DELETE_PATCH_CHANNEL_S;

/* Deleting statement to remove channel's facadeOut */
static const char DELETE_PATCH_CHANNEL_FACADE_OUT[] =
    "DELETE FROM SceneNodeInstOutput WHERE id=?1";
static sqlite3_stmt* DELETE_PATCH_CHANNEL_FACADE_OUT_S;

int
lsddb_deletePatchChannel (int chanId)
{
    /* First delete addresses */
    sqlite3_reset (CHANNEL_GET_ADDRS_S);
    sqlite3_bind_int (CHANNEL_GET_ADDRS_S, 1, chanId);

    if (sqlite3_step (CHANNEL_GET_ADDRS_S) == SQLITE_ROW)
    {
        int single = sqlite3_column_int (CHANNEL_GET_ADDRS_S, 0);
        int rAddrId = sqlite3_column_int (CHANNEL_GET_ADDRS_S, 1);
        lsddb_deletePatchChannelAddr (rAddrId);

        if (!single)
        {
            int gAddrId = sqlite3_column_int (CHANNEL_GET_ADDRS_S, 2);
            lsddb_deletePatchChannelAddr (gAddrId);
            int bAddrId = sqlite3_column_int (CHANNEL_GET_ADDRS_S, 3);
            lsddb_deletePatchChannelAddr (bAddrId);
        }

        /* Delete channel's facadeOut */
        int facadeOutId = sqlite3_column_int (CHANNEL_GET_ADDRS_S, 4);
        sqlite3_reset (DELETE_PATCH_CHANNEL_FACADE_OUT_S);
        sqlite3_bind_int (DELETE_PATCH_CHANNEL_FACADE_OUT_S, 1, facadeOutId);
        if (sqlite3_step (DELETE_PATCH_CHANNEL_FACADE_OUT_S) != SQLITE_DONE)
            doLog (ERROR, LOG_COMP, _("Unable to delete facadeOut in deletePatchChannel()."));
    }
    else
        doLog (ERROR, LOG_COMP, _("Unable to discover addresses in deletePatchChannel()."));

    /* Now Delete Channel */
    sqlite3_reset (DELETE_PATCH_CHANNEL_S);
    sqlite3_bind_int (DELETE_PATCH_CHANNEL_S, 1, chanId);

    if (sqlite3_step (DELETE_PATCH_CHANNEL_S) != SQLITE_DONE)
    {
        doLog (ERROR, LOG_COMP, _("Unable to delete channel in deletePatchChannel()."));
        return -1;
    }

    return 0;
}


/* Plugin API backend below */

/* Table creation/deletion */

static const char API_GET_PLUGIN_NAME[] =
    "SELECT pluginDomain FROM ScenePlugin WHERE id=?1";
static sqlite3_stmt* API_GET_PLUGIN_NAME_S;

static const char API_CHECK_PLUGIN_TABLE_REC[] =
    "SELECT tableName FROM ScenePluginTable WHERE pluginId=?1 AND tableName=?2";
static sqlite3_stmt* API_CHECK_PLUGIN_TABLE_REC_S;

static const char API_INSERT_PLUGIN_TABLE_REC[] =
    "INSERT INTO ScenePluginTable (pluginId,tableName) VALUES (?1,?2)";
static sqlite3_stmt* API_INSERT_PLUGIN_TABLE_REC_S;

/* Check to see if table exists already */
int
lsddbapi_verifyTable (int pluginId, const char* subName,
                      const unsigned char** bindName)
{
    if (!subName)  /* Improper use, no table */
    {
        if (bindName)
            *bindName = NULL;
        return 0;
    }

    /* We need the name of the plugin in order to prevent
     * name collisions */
    const unsigned char* pluginName;
    sqlite3_reset (API_GET_PLUGIN_NAME_S);
    sqlite3_bind_int (API_GET_PLUGIN_NAME_S, 1, pluginId);
    if (sqlite3_step (API_GET_PLUGIN_NAME_S) == SQLITE_ROW)
        pluginName = sqlite3_column_text (API_GET_PLUGIN_NAME_S, 0);
    else  /* Plugin doesn't exist, therefore no table */
    {
        doLog (ERROR, LOG_COMP, _("Unable to create plugin table, plugin non existent."));
        if (bindName)
            *bindName = NULL;
        return 0;
    }

    /* Using the plugin name, we make a composite name for
     * our individual table */
    char tableName[256];
    snprintf (tableName, 256, "%s_%s", pluginName, subName);

    /* Now we check to see if the table has been registered
     * in the past */
    /* based on its name and pluginId */
    sqlite3_reset (API_CHECK_PLUGIN_TABLE_REC_S);
    sqlite3_bind_int (API_CHECK_PLUGIN_TABLE_REC_S, 1, pluginId);
    sqlite3_bind_text (API_CHECK_PLUGIN_TABLE_REC_S, 2, tableName, -1, NULL);
    if (sqlite3_step (API_CHECK_PLUGIN_TABLE_REC_S) == SQLITE_ROW)  /*
                                                                     * Already
                                                                     * exists */
    {
        if (bindName)
            *bindName = sqlite3_column_text (API_CHECK_PLUGIN_TABLE_REC_S, 0);
        return 1;
    }
    else  /* Table doesn't exist; string binding will be set
           * to plugin name for convenience */
    {
        if (bindName)
            *bindName = pluginName;
        return 0;
    }
}


int
lsddbapi_createTable (int pluginId, const char* subName, const char* colDefs)
{
    if (!subName || !colDefs)
        return -1;

    const unsigned char* pluginName;
    if (lsddbapi_verifyTable (pluginId, subName, &pluginName)) /*
                                                                * Already
                                                                * exists */
        return 0;
    else if (pluginName)  /* Doesn't exist (but plugin
                           * does); make it! */

    {   /* Using the plugin name, we make a composite name
         * for our individual table */
        char tableName[256];
        snprintf (tableName, 256, "%s_%s", pluginName, subName);
        char tableStmt[512];
        snprintf (tableStmt,
                  512,
                  "CREATE TABLE IF NOT EXISTS %s (%s);",
                  tableName,
                  colDefs);

        /* Ensure there is only one semicolon to prevent SQL
         * injection attacks */
        const char* semicolon = strchr (tableStmt, ';');
        if (semicolon[1] != '\0')
        {
            doLog (ERROR, LOG_COMP, _("Two or more semicolons were detected in table creation statement, aborting."));
            return -1;
        }

        char* createErr;
        sqlite3_exec (memdb, tableStmt, NULL, NULL, &createErr);
        if (!createErr)
        {
            /* Successful creation, make record */
            sqlite3_reset (API_INSERT_PLUGIN_TABLE_REC_S);
            sqlite3_bind_int (API_INSERT_PLUGIN_TABLE_REC_S, 1, pluginId);
            sqlite3_bind_text (API_INSERT_PLUGIN_TABLE_REC_S,
                               2,
                               tableName,
                               -1,
                               NULL);
            if (sqlite3_step (API_INSERT_PLUGIN_TABLE_REC_S) == SQLITE_DONE)
                return 0;
            else
            {
                doLog (ERROR, LOG_COMP, _("Unable to insert table record."));
                return -1;
            }
        }
        else
        {
            doLog (ERROR, LOG_COMP, _("Unable to create table: %s."), createErr);
            sqlite3_free (createErr);
            return -1;
        }
    }
    else
    {
        doLog (ERROR, LOG_COMP, _("Plugin doesn't seem to exist for table creation."));
        return -1;
    }
}


int
lsddbapi_createIndex (int pluginId,
                      const char* idxName,
                      const char* subName,
                      const char* colDefs)
{
    if (!subName || !colDefs || !idxName)
        return -1;

    const unsigned char* pluginName;
    if (lsddbapi_verifyTable (pluginId, subName, &pluginName)) /*
                                                                * Already
                                                                * exists */
        return 0;
    else if (pluginName)  /* Doesn't exist (but plugin
                           * does); make it! */

    {   /* Using the plugin name, we make a composite name
         * for our individual table */
        char indexName[256];
        snprintf (indexName, 256, "%s_%s", pluginName, idxName);
        char tableName[256];
        snprintf (tableName, 256, "%s_%s", pluginName, subName);

        char indexStmt[512];
        snprintf (indexStmt,
                  512,
                  "CREATE INDEX IF NOT EXISTS %s ON %s (%s);",
                  indexName,
                  tableName,
                  colDefs);

        /* Ensure there is only one semicolon to prevent SQL
         * injection attacks */
        const char* semicolon = strchr (indexStmt, ';');
        if (semicolon[1] != '\0')
        {
            doLog (ERROR, LOG_COMP, _("Two or more semicolons were detected in table creation statement, aborting."));
            return -1;
        }

        char* createErr;
        sqlite3_exec (memdb, indexStmt, NULL, NULL, &createErr);
        if (!createErr)
            return 0;
        else
        {
            doLog (ERROR, LOG_COMP, _("Unable to create table index: %s."), createErr);
            sqlite3_free (createErr);
            return -1;
        }
    }
    else
    {
        doLog (ERROR, LOG_COMP, _("Plugin doesn't seem to exist for table index creation."));
        return -1;
    }
}


/* All prep functions below make a statement object, */
/* stores it internally, and provides an index to retrieve
 * it later */

int
lsddbapi_prepSelect (struct LSD_ScenePlugin const* pluginObj,
                     unsigned int* stmtIdxBind,
                     const char* tblName,
                     const char* colPortion,
                     const char* wherePortion)
{
    if (!pluginObj || !stmtIdxBind || !tblName || !colPortion || !wherePortion)
        return -1;

    /* First verify the table being accessed */
    const unsigned char* fullTableName;
    if (!lsddbapi_verifyTable (pluginObj->dbId, tblName, &fullTableName))
    {
        doLog (ERROR, LOG_COMP, _("Table being accessed doesn't exist, unable to prep."));
        return -1;
    }

    /* Compose the statement source */
    char stmtSource[256] = "";
    snprintf (stmtSource,
              256,
              "SELECT %s FROM %s WHERE %s",
              colPortion,
              fullTableName,
              wherePortion);

    /* Now prep the statement */
    sqlite3_stmt* newStmt;
    if (sqlite3_prepare_v2 (memdb, stmtSource, 256, &newStmt,
                            NULL) != SQLITE_OK)
    {
        doLog (ERROR, LOG_COMP, _("Sqlite was unable to prep a statement\nDetails: %s"),
                                  sqlite3_errmsg (memdb));
        return -1;
    }

    /* Insert the statement object into internal array */
    size_t newIdx;
    struct LSD_SceneDBStmt* newStmtObj;
    if (insertElem (getArr_lsdDBStmtArr (), &newIdx, (void**)&newStmtObj) < 0)
    {
        doLog (ERROR, LOG_COMP, _("Unable to insert new statement object into array."));
        return -1;
    }

    newStmtObj->pluginPtr = pluginObj;
    newStmtObj->stmt = newStmt;

    *stmtIdxBind = newIdx;

    return 0;
}


int
lsddbapi_prepInsert (struct LSD_ScenePlugin const* pluginObj,
                     unsigned int* stmtIdxBind,
                     const char* tblName,
                     const char* colPortion,
                     const char* valuesPortion)
{
    if (!pluginObj || !stmtIdxBind || !tblName || !colPortion || !valuesPortion)
        return -1;

    /* First verify the table being accessed */
    const unsigned char* fullTableName;
    if (!lsddbapi_verifyTable (pluginObj->dbId, tblName, &fullTableName))
    {
        doLog (ERROR, LOG_COMP, _("Table being accessed doesn't exist, unable to prep."));
        return -1;
    }

    /* Compose the statement source */
    char stmtSource[256] = "";
    snprintf (stmtSource,
              256,
              "INSERT INTO %s (%s) VALUES (%s)",
              fullTableName,
              colPortion,
              valuesPortion);

    /* Now prep the statement */
    sqlite3_stmt* newStmt;
    if (sqlite3_prepare_v2 (memdb, stmtSource, 256, &newStmt,
                            NULL) != SQLITE_OK)
    {
        doLog (ERROR, LOG_COMP, _("Sqlite was unable to prep a statement\nDetails: %s"),
                                  sqlite3_errmsg (memdb));
        return -1;
    }

    /* Insert the statement object into internal array */
    size_t newIdx;
    struct LSD_SceneDBStmt* newStmtObj;
    if (insertElem (getArr_lsdDBStmtArr (), &newIdx, (void**)&newStmtObj) < 0)
    {
        doLog (ERROR, LOG_COMP, _("Unable to insert new statement object into array."));
        return -1;
    }

    newStmtObj->pluginPtr = pluginObj;
    newStmtObj->stmt = newStmt;

    *stmtIdxBind = newIdx;

    return 0;
}


int
lsddbapi_prepUpdate (struct LSD_ScenePlugin const* pluginObj,
                     unsigned int* stmtIdxBind,
                     const char* tblName,
                     const char* setPortion,
                     const char* wherePortion)
{
    if (!pluginObj || !stmtIdxBind || !tblName || !setPortion || !wherePortion)
        return -1;

    /* First verify the table being accessed */
    const unsigned char* fullTableName;
    if (!lsddbapi_verifyTable (pluginObj->dbId, tblName, &fullTableName))
    {
        doLog (ERROR, LOG_COMP, _("Table being accessed doesn't exist, unable to prep."));
        return -1;
    }

    /* Compose the statement source */
    char stmtSource[256] = "";
    snprintf (stmtSource,
              256,
              "UPDATE %s SET %s WHERE %s",
              fullTableName,
              setPortion,
              wherePortion);

    /* Now prep the statement */
    sqlite3_stmt* newStmt;
    if (sqlite3_prepare_v2 (memdb, stmtSource, 256, &newStmt,
                            NULL) != SQLITE_OK)
    {
        doLog (ERROR, LOG_COMP, _("Sqlite was unable to prep a statement\nDetails: %s"),
                                  sqlite3_errmsg (memdb));
        return -1;
    }

    /* Insert the statement object into internal array */
    size_t newIdx;
    struct LSD_SceneDBStmt* newStmtObj;
    if (insertElem (getArr_lsdDBStmtArr (), &newIdx, (void**)&newStmtObj) < 0)
    {
        doLog (ERROR, LOG_COMP, _("Unable to insert new statement object into array."));
        return -1;
    }

    newStmtObj->pluginPtr = pluginObj;
    newStmtObj->stmt = newStmt;

    *stmtIdxBind = newIdx;

    return 0;
}


int
lsddbapi_prepDelete (struct LSD_ScenePlugin const* pluginObj,
                     unsigned int* stmtIdxBind,
                     const char* tblName,
                     const char* wherePortion)
{
    if (!pluginObj || !stmtIdxBind || !tblName || !wherePortion)
        return -1;

    /* First verify the table being accessed */
    const unsigned char* fullTableName;
    if (!lsddbapi_verifyTable (pluginObj->dbId, tblName, &fullTableName))
    {
        doLog (ERROR, LOG_COMP, _("Table being accessed doesn't exist, unable to prep."));
        return -1;
    }

    /* Compose the statement source */
    char stmtSource[256] = "";
    snprintf (stmtSource,
              256,
              "DELETE FROM %s WHERE %s",
              fullTableName,
              wherePortion);

    /* Now prep the statement */
    sqlite3_stmt* newStmt;
    if (sqlite3_prepare_v2 (memdb, stmtSource, 256, &newStmt,
                            NULL) != SQLITE_OK)
    {
        doLog (ERROR, LOG_COMP, _("Sqlite was unable to prep a statement\nDetails: %s"),
                                  sqlite3_errmsg (memdb));
        return -1;
    }

    /* Insert the statement object into internal array */
    size_t newIdx;
    struct LSD_SceneDBStmt* newStmtObj;
    if (insertElem (getArr_lsdDBStmtArr (), &newIdx, (void**)&newStmtObj) < 0)
    {
        doLog (ERROR, LOG_COMP, _("Unable to insert new statement object into array."));
        return -1;
    }

    newStmtObj->pluginPtr = pluginObj;
    newStmtObj->stmt = newStmt;

    *stmtIdxBind = newIdx;

    return 0;
}

const char* PICK_STMT_ERR()
{
    return _("Unable to pick stmt from array.");
}
               
const char* STMT_OWN_ERR()
{
    return _("Unable to verify ownership of stmt.");
}

int
lsddbapi_stmtReset (struct LSD_ScenePlugin const* plugin,
                    unsigned int stmtIdx,
                    int* result)
{
    /* Pick stmt object and verify plugin ownership */
    struct LSD_SceneDBStmt* stmtObj;
    if (pickIdx (getArr_lsdDBStmtArr (), (void**)&stmtObj, stmtIdx) < 0)
    {
        doLog (ERROR, LOG_COMP, PICK_STMT_ERR());
        return -1;
    }

    if (plugin == stmtObj->pluginPtr)
        *result = sqlite3_reset (stmtObj->stmt);
    else
    {
        doLog (ERROR, LOG_COMP, STMT_OWN_ERR());
        return -1;
    }

    return 0;
}


int
lsddbapi_stmtStep (struct LSD_ScenePlugin const* plugin,
                   unsigned int stmtIdx,
                   int* result)
{
    /* Pick stmt object and verify plugin ownership */
    struct LSD_SceneDBStmt* stmtObj;
    if (pickIdx (getArr_lsdDBStmtArr (), (void**)&stmtObj, stmtIdx) < 0)
    {
        doLog (ERROR, LOG_COMP, PICK_STMT_ERR());
        return -1;
    }

    if (plugin == stmtObj->pluginPtr)
        *result = sqlite3_step (stmtObj->stmt);
    else
    {
        doLog (ERROR, LOG_COMP, STMT_OWN_ERR());
        return -1;
    }

    return 0;
}


int
lsddbapi_stmtBindDouble (struct LSD_ScenePlugin const* plugin,
                         unsigned int stmtIdx,
                         unsigned int sqlIdx,
                         double data,
                         int* result)
{
    /* Pick stmt object and verify plugin ownership */
    struct LSD_SceneDBStmt* stmtObj;
    if (pickIdx (getArr_lsdDBStmtArr (), (void**)&stmtObj, stmtIdx) < 0)
    {
        doLog (ERROR, LOG_COMP, PICK_STMT_ERR());
        return -1;
    }

    if (plugin == stmtObj->pluginPtr)
        *result = sqlite3_bind_double (stmtObj->stmt, sqlIdx, data);
    else
    {
        doLog (ERROR, LOG_COMP, STMT_OWN_ERR());
        return -1;
    }

    return 0;
}


int
lsddbapi_stmtBindInt (struct LSD_ScenePlugin const* plugin,
                      unsigned int stmtIdx,
                      unsigned int sqlIdx,
                      int data,
                      int* result)
{
    /* Pick stmt object and verify plugin ownership */
    struct LSD_SceneDBStmt* stmtObj;
    if (pickIdx (getArr_lsdDBStmtArr (), (void**)&stmtObj, stmtIdx) < 0)
    {
        doLog (ERROR, LOG_COMP, PICK_STMT_ERR());
        return -1;
    }

    if (plugin == stmtObj->pluginPtr)
        *result = sqlite3_bind_int (stmtObj->stmt, sqlIdx, data);
    else
    {
        doLog (ERROR, LOG_COMP, STMT_OWN_ERR());
        return -1;
    }

    return 0;
}


int
lsddbapi_stmtBindInt64 (struct LSD_ScenePlugin const* plugin,
                        unsigned int stmtIdx,
                        unsigned int sqlIdx,
                        sqlite3_int64 data,
                        int* result)
{
    /* Pick stmt object and verify plugin ownership */
    struct LSD_SceneDBStmt* stmtObj;
    if (pickIdx (getArr_lsdDBStmtArr (), (void**)&stmtObj, stmtIdx) < 0)
    {
        doLog (ERROR, LOG_COMP, PICK_STMT_ERR());
        return -1;
    }

    if (plugin == stmtObj->pluginPtr)
        *result = sqlite3_bind_int64 (stmtObj->stmt, sqlIdx, data);
    else
    {
        doLog (ERROR, LOG_COMP, STMT_OWN_ERR());
        return -1;
    }

    return 0;
}


int
lsddbapi_stmtBindNull (struct LSD_ScenePlugin const* plugin,
                       unsigned int stmtIdx,
                       unsigned int sqlIdx,
                       int* result)
{
    /* Pick stmt object and verify plugin ownership */
    struct LSD_SceneDBStmt* stmtObj;
    if (pickIdx (getArr_lsdDBStmtArr (), (void**)&stmtObj, stmtIdx) < 0)
    {
        doLog (ERROR, LOG_COMP, PICK_STMT_ERR());
        return -1;
    }

    if (plugin == stmtObj->pluginPtr)
        *result = sqlite3_bind_null (stmtObj->stmt, sqlIdx);
    else
    {
        doLog (ERROR, LOG_COMP, STMT_OWN_ERR());
        return -1;
    }

    return 0;
}


int
lsddbapi_stmtBindText (struct LSD_ScenePlugin const* plugin,
                       unsigned int stmtIdx,
                       unsigned int sqlIdx,
                       const char* data,
                       int length,
                       void ( *destructor )(void*),
                       int* result)
{
    /* Pick stmt object and verify plugin ownership */
    struct LSD_SceneDBStmt* stmtObj;
    if (pickIdx (getArr_lsdDBStmtArr (), (void**)&stmtObj, stmtIdx) < 0)
    {
        doLog (ERROR, LOG_COMP, PICK_STMT_ERR());
        return -1;
    }

    if (plugin == stmtObj->pluginPtr)
        *result = sqlite3_bind_text (stmtObj->stmt,
                                     sqlIdx,
                                     data,
                                     length,
                                     destructor);
    else
    {
        doLog (ERROR, LOG_COMP, STMT_OWN_ERR());
        return -1;
    }

    return 0;
}


int
lsddbapi_stmtBindText16 (struct LSD_ScenePlugin const* plugin,
                         unsigned int stmtIdx,
                         unsigned int sqlIdx,
                         const void* data,
                         int length,
                         void ( *destructor )(void*),
                         int* result)
{
    /* Pick stmt object and verify plugin ownership */
    struct LSD_SceneDBStmt* stmtObj;
    if (pickIdx (getArr_lsdDBStmtArr (), (void**)&stmtObj, stmtIdx) < 0)
    {
        doLog (ERROR, LOG_COMP, PICK_STMT_ERR());
        return -1;
    }

    if (plugin == stmtObj->pluginPtr)
        *result = sqlite3_bind_text16 (stmtObj->stmt,
                                       sqlIdx,
                                       data,
                                       length,
                                       destructor);
    else
    {
        doLog (ERROR, LOG_COMP, STMT_OWN_ERR());
        return -1;
    }

    return 0;
}


int
lsddbapi_stmtColDouble (struct LSD_ScenePlugin const* plugin,
                        unsigned int stmtIdx,
                        unsigned int colIdx,
                        double* result)
{
    /* Pick stmt object and verify plugin ownership */
    struct LSD_SceneDBStmt* stmtObj;
    if (pickIdx (getArr_lsdDBStmtArr (), (void**)&stmtObj, stmtIdx) < 0)
    {
        doLog (ERROR, LOG_COMP, PICK_STMT_ERR());
        return -1;
    }

    if (plugin == stmtObj->pluginPtr)
        *result = sqlite3_column_double (stmtObj->stmt, colIdx);
    else
    {
        doLog (ERROR, LOG_COMP, STMT_OWN_ERR());
        return -1;
    }

    return 0;
}


int
lsddbapi_stmtColInt (struct LSD_ScenePlugin const* plugin, unsigned int stmtIdx,
                     unsigned int colIdx, int* result)
{
    /* Pick stmt object and verify plugin ownership */
    struct LSD_SceneDBStmt* stmtObj;
    if (pickIdx (getArr_lsdDBStmtArr (), (void**)&stmtObj, stmtIdx) < 0)
    {
        doLog (ERROR, LOG_COMP, PICK_STMT_ERR());
        return -1;
    }

    if (plugin == stmtObj->pluginPtr)
        *result = sqlite3_column_int (stmtObj->stmt, colIdx);
    else
    {
        doLog (ERROR, LOG_COMP, STMT_OWN_ERR());
        return -1;
    }

    return 0;
}


int
lsddbapi_stmtColInt64 (struct LSD_ScenePlugin const* plugin,
                       unsigned int stmtIdx,
                       unsigned int colIdx,
                       sqlite3_int64* result)
{
    /* Pick stmt object and verify plugin ownership */
    struct LSD_SceneDBStmt* stmtObj;
    if (pickIdx (getArr_lsdDBStmtArr (), (void**)&stmtObj, stmtIdx) < 0)
    {
        doLog (ERROR, LOG_COMP, PICK_STMT_ERR());
        return -1;
    }

    if (plugin == stmtObj->pluginPtr)
        *result = sqlite3_column_int64 (stmtObj->stmt, colIdx);
    else
    {
        doLog (ERROR, LOG_COMP, STMT_OWN_ERR());
        return -1;
    }

    return 0;
}


int
lsddbapi_stmtColText (struct LSD_ScenePlugin const* plugin,
                      unsigned int stmtIdx,
                      unsigned int colIdx,
                      const unsigned char** result)
{
    /* Pick stmt object and verify plugin ownership */
    struct LSD_SceneDBStmt* stmtObj;
    if (pickIdx (getArr_lsdDBStmtArr (), (void**)&stmtObj, stmtIdx) < 0)
    {
        doLog (ERROR, LOG_COMP, PICK_STMT_ERR());
        return -1;
    }

    if (plugin == stmtObj->pluginPtr)
        *result = sqlite3_column_text (stmtObj->stmt, colIdx);
    else
    {
        doLog (ERROR, LOG_COMP, STMT_OWN_ERR());
        return -1;
    }

    return 0;
}


int
lsddbapi_stmtColText16 (struct LSD_ScenePlugin const* plugin,
                        unsigned int stmtIdx,
                        unsigned int colIdx,
                        const void** result)
{
    /* Pick stmt object and verify plugin ownership */
    struct LSD_SceneDBStmt* stmtObj;
    if (pickIdx (getArr_lsdDBStmtArr (), (void**)&stmtObj, stmtIdx) < 0)
    {
        doLog (ERROR, LOG_COMP, PICK_STMT_ERR());
        return -1;
    }

    if (plugin == stmtObj->pluginPtr)
        *result = sqlite3_column_text16 (stmtObj->stmt, colIdx);
    else
    {
        doLog (ERROR, LOG_COMP, STMT_OWN_ERR());
        return -1;
    }

    return 0;
}


int
lsddbapi_getLastInsertRowId ()
{
    return sqlite3_last_insert_rowid (memdb);
}


/* General database things below */
void
lsddb_reportPrepProblem (int problem)
{
    doLog (ERROR, LOG_COMP, _("Prep problem: %d\nDetails: %s"), problem,
             sqlite3_errmsg (memdb));
}


#define PREP(stmt, \
             num)         if (sqlite3_prepare_v2 (memdb, stmt, -1, &stmt ## _S, \
                                                  NULL) != \
                              SQLITE_OK) {lsddb_reportPrepProblem (num); \
                                          return -1; }
#define FINAL(stmt)        sqlite3_finalize (stmt ## _S)

/* The numbers in these macros are used to aid debugging; */
/* they serve no functional purpose */

int
lsddb_prepStmts ()
{
    PREP (CREATE_PATCH_SPACE, 1);

    PREP (REMOVE_PATCH_SPACE, 2);
    PREP (REMOVE_PATCH_SPACE_NODES, 3);
    PREP (REMOVE_PATCH_SPACE_FACADES, 33);
    PREP (REMOVE_PATCH_SPACE_INS, 4);
    PREP (REMOVE_PATCH_SPACE_OUTS, 5);

    PREP (UPDATE_PATCH_SPACE_NAME, 55);
    PREP (SET_PS_COLOUR, 555);

    PREP (CREATE_PATCH_SPACE_IN, 6);
    PREP (CREATE_PATCH_SPACE_OUT, 7);
    PREP (REMOVE_PATCH_SPACE_OUT, 777);
    PREP (REMOVE_PATCH_SPACE_IN, 888);
    PREP (UPDATE_PATCH_SPACE_IN_NAME, 999);
    PREP (UPDATE_PATCH_SPACE_OUT_NAME, 111);

    PREP (CREATE_PARTITION, 8);

    PREP (SET_PARTITION_IMAGE, 9);

    PREP (REMOVE_PARTITON, 10);
    PREP (GET_PARTITON_PATCHSPACE, 11);
    PREP (REMOVE_PARTITON_GET_CHANNELS, 12);

    PREP (UPDATE_PARTITON_NAME, 13);
    PREP (UPDATE_PARTITON_PS_NAME, 14);

    PREP (ADD_NODE_CLASS_CHECK, 15);
    PREP (ADD_NODE_CLASS_INSERT, 16);
    PREP (ADD_NODE_CLASS_UPDIDX, 17);

    PREP (ADD_DATA_TYPE_CHECK, 18);
    PREP (ADD_DATA_TYPE_INSERT, 19);

    PREP (STRUCT_NODE_INST_OUTPUT_ARR, 20);
    PREP (STRUCT_NODE_INST_OUTPUT_ARR_UPDIDX, 21);

    PREP (STRUCT_NODE_INST_INPUT_ARR, 22);
    PREP (STRUCT_NODE_INST_INPUT_ARR_UPDIDX, 23);

    PREP (STRUCT_NODE_INST_ARR_CHECK_ENABLE, 123);
    PREP (CHECK_INPUT_ENABLE, 124);
    PREP (CHECK_OUTPUT_ENABLE, 125);
    PREP (STRUCT_NODE_INST_ARR, 24);
    PREP (STRUCT_NODE_INST_ARR_UPDIDX, 25);

    PREP (STRUCT_UNIV_ARR, 26);
    PREP (STRUCT_UNIV_ARR_UPDIDX, 27);
    PREP (STRUCT_UNIV_ARR_MAXIDX, 28);

    PREP (STRUCT_CHANNEL_ARR_ADDR, 29);

    PREP (STRUCT_CHANNEL_ARR, 30);
    PREP (STRUCT_CHANNEL_ARR_UPDIDX, 31);

    PREP (STRUCT_PARTITION_ARR, 32);
    PREP (STRUCT_PARTITION_ARR_UPDIDX, 33);

    PREP (ADD_NODE_INST_INPUT_INSERT, 34);
    PREP (ADD_NODE_INST_INPUT_UPDIDX, 35);

    PREP (ADD_NODE_INST_OUTPUT_INSERT, 36);
    PREP (ADD_NODE_INST_OUTPUT_UPDIDX, 37);

    PREP (REMOVE_NODE_INST_INPUT_ARRIDX, 38);
    PREP (REMOVE_NODE_INST_INPUT, 39);
    PREP (REMOVE_NODE_INST_INPUT_GET_WIRES, 40);

    PREP (REMOVE_NODE_INST_OUTPUT_ARRIDX, 41);
    PREP (REMOVE_NODE_INST_OUTPUT, 42);
    PREP (REMOVE_NODE_INST_OUTPUT_GET_WIRES, 43);

    PREP (ADD_NODE_INST, 44);
    PREP (GET_NODE_CLASS_NAME, 444);

    PREP (REMOVE_NODE_INST_CHECK, 45);
    PREP (REMOVE_NODE_INST_GET_INS, 46);
    PREP (REMOVE_NODE_INST_GET_OUTS, 47);
    PREP (REMOVE_NODE_INST_DELETE, 48);

    PREP (UPDATE_NODE_NAME, 488);
    PREP (SET_NODE_COLOUR, 489);

    PREP (NODE_INST_POS, 49);

    PREP (FACADE_INST_POS, 50);

    PREP (PAN_PATCH_SPACE, 51);

    PREP (INDEX_HTML_GEN, 151);

    PREP (JSON_PLUGINS, 051);
    PREP (DISABLE_PLUGIN, 151);
    PREP (ENABLE_PLUGIN, 251);

    PREP (PLUGIN_HEAD_LOADER_CHECK_NAME, 52);
    PREP (PLUGIN_HEAD_LOADER_CHECK_SHA, 152);
    PREP (PLUGIN_HEAD_LOADER_UPDATE_SHA, 252);
    PREP (PLUGIN_HEAD_LOADER_SEEN, 53);
    PREP (PLUGIN_HEAD_LOADER_INSERT, 54);
    PREP (PLUGIN_HEAD_LOADER_UPDIDX_LOAD, 55);

    PREP (RESOLVE_PLUGIN_FROM_NODE, 555);

    PREP (TRACE_INPUT, 56);

    PREP (TRACE_OUTPUT, 57);

    PREP (CHECK_CHANNEL_WIRING_GET_PS, 58);
    PREP (CHECK_CHANNEL_WIRING_GET_CHAN_ARRIDX, 59);

    PREP (WIRE_NODES_CHECK_FACADE_INT_OUT, 599);
    PREP (WIRE_NODES_CHECK_SRC_OUT, 60);
    PREP (WIRE_NODES_CHECK_DEST_IN, 61);
    PREP (WIRE_NODES_SET_FACADE_IN_DATA, 62);
    PREP (WIRE_NODES_SET_FACADE_OUT_DATA, 63);
    PREP (WIRE_NODES_GET_FACADE_IN_CPS, 64);
    PREP (WIRE_NODES_GET_FACADE_OUT_CPS, 65);
    PREP (WIRE_NODES_GET_IN_PS, 66);
    PREP (WIRE_NODES_GET_OUT_PS, 67);
    PREP (WIRE_NODES, 68);

    PREP (UNWIRE_NODES_GET_EDGE_DETAILS, 69);
    PREP (UNWIRE_NODES_DELETE_EDGE, 70);
    PREP (UNWIRE_NODES_GET_SRC_FACADE_EDGE, 71);
    PREP (UNWIRE_NODES_GET_DEST_FACADE_EDGE, 72);
    PREP (UNWIRE_FACADE_OUT_ALIAS, 722);

    PREP (REWIRE_NODES, 777);
    PREP (REWIRE_NODES_DEST, 778);

    PREP (JSON_CLASS_LIBRARY, 73);

    PREP (JSON_GET_FACADE_OUTS, 74);

    PREP (JSON_GET_FACADE_INS, 75);

    PREP (JSON_PARTS, 76);

    PREP (JSON_INSERT_CLASS_OBJECT, 766);

    PREP (JSON_NODES, 77);
    PREP (JSON_NODES_INS, 78);
    PREP (JSON_NODES_OUTS, 79);

    PREP (JSON_NODES_FACADES, 80);
    PREP (JSON_NODES_FACADES_INS, 81);
    PREP (JSON_NODES_FACADES_OUTS, 82);

    PREP (JSON_GET_FACADE, 882);

    PREP (JSON_WIRES, 83);

    PREP (JSON_PATCH_SPACE, 84);

    PREP (RESOLVE_CLASS_FROM_ID, 85);
    PREP (RESOLVE_INST_FROM_ID, 885);
    PREP (RESOLVE_INST_FROM_IN_ID, 886);
    PREP (RESOLVE_INST_FROM_OUT_ID, 887);

    PREP (GET_PATCH_CHANNELS_PARTS, 87);
    PREP (GET_PATCH_CHANNELS_CHANS, 88);
    PREP (GET_PATCH_CHANNELS_ADDITIONAL, 89);

    PREP (ADD_PATCH_CHANNEL, 90);
    PREP (ADD_PATCH_CHANNEL_ADDR, 91);
    PREP (ADD_PATCH_CHANNEL_FACADE_OUT, 92);

    PREP (CHANNEL_GET_ADDRS, 93);
    PREP (UPDATE_PATCH_CHANNEL, 94);
    PREP (DELETE_PATCH_CHANNEL_ADDR, 95);

    PREP (UPDATE_PATCH_CHANNEL_FACADE_OUT, 96);

    PREP (DELETE_PATCH_CHANNEL, 97);
    PREP (DELETE_PATCH_CHANNEL_FACADE_OUT, 98);

    PREP (API_GET_PLUGIN_NAME, 99);
    PREP (API_CHECK_PLUGIN_TABLE_REC, 100);
    PREP (API_INSERT_PLUGIN_TABLE_REC, 101);

    return 0;
}


int
lsddb_finishStmts ()
{
    FINAL (CREATE_PATCH_SPACE);

    FINAL (REMOVE_PATCH_SPACE);
    FINAL (REMOVE_PATCH_SPACE_NODES);
    FINAL (REMOVE_PATCH_SPACE_FACADES);
    FINAL (REMOVE_PATCH_SPACE_INS);
    FINAL (REMOVE_PATCH_SPACE_OUTS);

    FINAL (UPDATE_PATCH_SPACE_NAME);
    FINAL (SET_PS_COLOUR);

    FINAL (CREATE_PATCH_SPACE_IN);
    FINAL (CREATE_PATCH_SPACE_OUT);
    FINAL (REMOVE_PATCH_SPACE_OUT);
    FINAL (REMOVE_PATCH_SPACE_IN);
    FINAL (UPDATE_PATCH_SPACE_IN_NAME);
    FINAL (UPDATE_PATCH_SPACE_OUT_NAME);

    FINAL (CREATE_PARTITION);

    FINAL (SET_PARTITION_IMAGE);

    FINAL (REMOVE_PARTITON);
    FINAL (GET_PARTITON_PATCHSPACE);
    FINAL (REMOVE_PARTITON_GET_CHANNELS);

    FINAL (UPDATE_PARTITON_NAME);
    FINAL (UPDATE_PARTITON_PS_NAME);

    FINAL (ADD_NODE_CLASS_CHECK);
    FINAL (ADD_NODE_CLASS_INSERT);
    FINAL (ADD_NODE_CLASS_UPDIDX);

    FINAL (ADD_DATA_TYPE_CHECK);
    FINAL (ADD_DATA_TYPE_INSERT);

    FINAL (STRUCT_NODE_INST_OUTPUT_ARR);
    FINAL (STRUCT_NODE_INST_OUTPUT_ARR_UPDIDX);

    FINAL (STRUCT_NODE_INST_INPUT_ARR);
    FINAL (STRUCT_NODE_INST_INPUT_ARR_UPDIDX);

    FINAL (STRUCT_NODE_INST_ARR_CHECK_ENABLE);
    FINAL (CHECK_INPUT_ENABLE);
    FINAL (CHECK_OUTPUT_ENABLE);
    FINAL (STRUCT_NODE_INST_ARR);
    FINAL (STRUCT_NODE_INST_ARR_UPDIDX);

    FINAL (STRUCT_UNIV_ARR);
    FINAL (STRUCT_UNIV_ARR_UPDIDX);
    FINAL (STRUCT_UNIV_ARR_MAXIDX);

    FINAL (STRUCT_CHANNEL_ARR_ADDR);

    FINAL (STRUCT_CHANNEL_ARR);
    FINAL (STRUCT_CHANNEL_ARR_UPDIDX);

    FINAL (STRUCT_PARTITION_ARR);
    FINAL (STRUCT_PARTITION_ARR_UPDIDX);

    FINAL (ADD_NODE_INST_INPUT_INSERT);
    FINAL (ADD_NODE_INST_INPUT_UPDIDX);

    FINAL (ADD_NODE_INST_OUTPUT_INSERT);
    FINAL (ADD_NODE_INST_OUTPUT_UPDIDX);

    FINAL (REMOVE_NODE_INST_INPUT_ARRIDX);
    FINAL (REMOVE_NODE_INST_INPUT);
    FINAL (REMOVE_NODE_INST_INPUT_GET_WIRES);

    FINAL (REMOVE_NODE_INST_OUTPUT_ARRIDX);
    FINAL (REMOVE_NODE_INST_OUTPUT);
    FINAL (REMOVE_NODE_INST_OUTPUT_GET_WIRES);

    FINAL (ADD_NODE_INST);
    FINAL (GET_NODE_CLASS_NAME);

    FINAL (REMOVE_NODE_INST_CHECK);
    FINAL (REMOVE_NODE_INST_GET_INS);
    FINAL (REMOVE_NODE_INST_GET_OUTS);
    FINAL (REMOVE_NODE_INST_DELETE);

    FINAL (UPDATE_NODE_NAME);
    FINAL (SET_NODE_COLOUR);

    FINAL (NODE_INST_POS);

    FINAL (FACADE_INST_POS);

    FINAL (PAN_PATCH_SPACE);

    FINAL (INDEX_HTML_GEN);

    FINAL (JSON_PLUGINS);
    FINAL (DISABLE_PLUGIN);
    FINAL (ENABLE_PLUGIN);

    FINAL (PLUGIN_HEAD_LOADER_CHECK_NAME);
    FINAL (PLUGIN_HEAD_LOADER_CHECK_SHA);
    FINAL (PLUGIN_HEAD_LOADER_UPDATE_SHA);
    FINAL (PLUGIN_HEAD_LOADER_SEEN);
    FINAL (PLUGIN_HEAD_LOADER_INSERT);
    FINAL (PLUGIN_HEAD_LOADER_UPDIDX_LOAD);

    FINAL (RESOLVE_PLUGIN_FROM_NODE);

    FINAL (TRACE_INPUT);

    FINAL (TRACE_OUTPUT);

    FINAL (CHECK_CHANNEL_WIRING_GET_PS);
    FINAL (CHECK_CHANNEL_WIRING_GET_CHAN_ARRIDX);

    FINAL (WIRE_NODES_CHECK_FACADE_INT_OUT);
    FINAL (WIRE_NODES_CHECK_SRC_OUT);
    FINAL (WIRE_NODES_CHECK_DEST_IN);
    FINAL (WIRE_NODES_SET_FACADE_IN_DATA);
    FINAL (WIRE_NODES_SET_FACADE_OUT_DATA);
    FINAL (WIRE_NODES_GET_FACADE_IN_CPS);
    FINAL (WIRE_NODES_GET_FACADE_OUT_CPS);
    FINAL (WIRE_NODES_GET_IN_PS);
    FINAL (WIRE_NODES_GET_OUT_PS);
    FINAL (WIRE_NODES);

    FINAL (UNWIRE_NODES_GET_EDGE_DETAILS);
    FINAL (UNWIRE_NODES_DELETE_EDGE);
    FINAL (UNWIRE_NODES_GET_SRC_FACADE_EDGE);
    FINAL (UNWIRE_NODES_GET_DEST_FACADE_EDGE);
    FINAL (UNWIRE_FACADE_OUT_ALIAS);

    FINAL (REWIRE_NODES);
    FINAL (REWIRE_NODES_DEST);

    FINAL (JSON_CLASS_LIBRARY);

    FINAL (JSON_GET_FACADE_OUTS);

    FINAL (JSON_GET_FACADE_INS);

    FINAL (JSON_PARTS);

    FINAL (JSON_INSERT_CLASS_OBJECT);

    FINAL (JSON_NODES);
    FINAL (JSON_NODES_INS);
    FINAL (JSON_NODES_OUTS);

    FINAL (JSON_NODES_FACADES);
    FINAL (JSON_NODES_FACADES_INS);
    FINAL (JSON_NODES_FACADES_OUTS);

    FINAL (JSON_GET_FACADE);

    FINAL (JSON_WIRES);

    FINAL (JSON_PATCH_SPACE);

    FINAL (RESOLVE_CLASS_FROM_ID);
    FINAL (RESOLVE_INST_FROM_ID);
    FINAL (RESOLVE_INST_FROM_IN_ID);
    FINAL (RESOLVE_INST_FROM_OUT_ID);

    FINAL (GET_PATCH_CHANNELS_PARTS);
    FINAL (GET_PATCH_CHANNELS_CHANS);
    FINAL (GET_PATCH_CHANNELS_ADDITIONAL);

    FINAL (ADD_PATCH_CHANNEL);
    FINAL (ADD_PATCH_CHANNEL_ADDR);
    FINAL (ADD_PATCH_CHANNEL_FACADE_OUT);

    FINAL (CHANNEL_GET_ADDRS);
    FINAL (UPDATE_PATCH_CHANNEL);
    FINAL (DELETE_PATCH_CHANNEL_ADDR);

    FINAL (UPDATE_PATCH_CHANNEL_FACADE_OUT);

    FINAL (DELETE_PATCH_CHANNEL);
    FINAL (DELETE_PATCH_CHANNEL_FACADE_OUT);

    FINAL (API_GET_PLUGIN_NAME);
    FINAL (API_CHECK_PLUGIN_TABLE_REC);
    FINAL (API_INSERT_PLUGIN_TABLE_REC);

    return 0;
}


