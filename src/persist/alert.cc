/*
Copyright (C) 2014-2015 Eaton
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*! \file alert.cc
    \brief Pure DB API for CRUD operations on alerts

    \author Alena Chernikava <alenachernikava@eaton.com>
*/

#include <tntdb/row.h>
#include <tntdb/result.h>
#include <tntdb/error.h>
#include <tntdb/transaction.h>

#include "log.h"
#include "defs.h"
#include "alert.h"

//=============================================================================
// end date of the alert can't be specified during the insert statement
db_reply_t
    insert_into_alert 
        (tntdb::Connection  &conn,
         const char         *rule_name,
         a_elmnt_pr_t        priority,
         m_alrt_state_t      alert_state,
         const char         *description,
         m_alrt_ntfctn_t     notification,
         int64_t             date_from)
{
    LOG_START;
    log_debug ("  rule_name = '%s'", rule_name);
    log_debug ("  priority = %" PRIu16, priority);
    log_debug ("  alert_state = %" PRIu16, alert_state);
    log_debug ("  description = '%s'", description);
    log_debug ("  date_from = %" PRIi64, date_from);
    log_debug ("  notification = %" PRIu16, notification);

    db_reply_t ret = db_reply_new();

    // input parameters control 
    if ( !is_ok_rule_name (rule_name) )
    {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "rule name is invalid";
        log_error ("end: %s, %s","ignore insert", ret.msg);
        return ret;
    }
    if ( !is_ok_priority (priority) )
    {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "unsupported value of priority";
        log_error ("end: %s, %s", "ignore insert", ret.msg);
        return ret;
    }
    if ( !is_ok_alert_state (alert_state) )
    {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "alert state is invalid";
        log_error ("end: %s, %s","ignore insert", ret.msg);
        return ret;
    }
    // description can be even NULL
    // notification can be any number. It is treated as a bit-vector
    // ATTENTION: no control for time
    log_debug ("input parameters are correct");
    
    try{
        tntdb::Statement st = conn.prepareCached(
            " INSERT INTO"
            "   t_bios_alert"
            "   (rule_name, priority, state, descriprion,"
            "   notification, date_from)"
            " SELECT"
            "    :rule, :priority, :state, :desc, :note, FROM_UNIXTIME(:from)"
            " FROM"
            "   t_empty"
            " WHERE NOT EXISTS"
            "   ("
            "       SELECT"
            "           id"
            "       FROM"
            "           t_bios_alert v"
            "       WHERE"
            "           v.date_till is NULL AND"
            "           v.rule_name = :rule"
            "   )"
        );
   
        ret.affected_rows = st.set("rule", rule_name).
                               set("priority", priority).
                               set("state", alert_state).
                               set("desc", description).
                               set("note", notification).
                               set("from", date_from).
                               execute();
        ret.rowid = conn.lastInsertId();
        log_debug ("[t_bios_alert]: was inserted %" 
                                    PRIu64 " rows", ret.affected_rows);
        ret.status = 1;
        LOG_END;
        return ret;
    }
    catch (const std::exception &e) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = e.what();
        LOG_END_ABNORMAL(e);
        return ret;
    }
}


//=============================================================================

db_reply_t
    update_alert_notification 
        (tntdb::Connection  &conn,
         m_alrt_ntfctn_t     notification,
         m_alrt_id_t         id)
{
    LOG_START;
    log_debug ("  notification = %" PRIu16, notification);
    log_debug ("  id = %" PRIu32, id);

    db_reply_t ret = db_reply_new();
    
    try{
        tntdb::Statement st = conn.prepareCached(
            " UPDATE"
            "   t_bios_alert"
            " SET notification = (notification | :note)"  // a bitwire OR
            " WHERE  id = :id"
        );
   
        ret.affected_rows = st.set("id", id).
                               set("note", notification).
                               execute();
        ret.rowid = conn.lastInsertId();
        log_debug ("[t_bios_alert]: was updated %" 
                                    PRIu64 " rows", ret.affected_rows);
        ret.status = 1;
        LOG_END;
        return ret;
    }
    catch (const std::exception &e) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = e.what();
        LOG_END_ABNORMAL(e);
        return ret;
    }
}


//=============================================================================
db_reply_t
    update_alert_tilldate 
        (tntdb::Connection  &conn,
         int64_t             date_till,
         m_alrt_id_t         id)
{
    LOG_START;
    log_debug ("  tilldate = %" PRIi64, date_till);
    log_debug ("  id = %" PRIu32, id);

    db_reply_t ret = db_reply_new();
    
    try{
        tntdb::Statement st = conn.prepareCached(
            " UPDATE"
            "   t_bios_alert"
            " SET date_till = FROM_UNIXTIME(:till)"
            " WHERE  id = :id"
        );
   
        ret.affected_rows = st.set("id", id).
                               set("till", date_till).
                               execute();
        ret.rowid = conn.lastInsertId();
        log_debug ("[t_bios_alert]: was updated %" 
                                    PRIu64 " rows", ret.affected_rows);
        ret.status = 1;
        LOG_END;
        return ret;
    }
    catch (const std::exception &e) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = e.what();
        LOG_END_ABNORMAL(e);
        return ret;
    }
}

//=============================================================================
db_reply_t
    delete_from_alert
        (tntdb::Connection &conn,
         m_alrt_id_t id)
{
    LOG_START;
    log_debug ("  id = %" PRIu32, id);

    db_reply_t ret = db_reply_new();

    try{
        tntdb::Statement st = conn.prepareCached(
            " DELETE FROM"
            "   t_bios_alert"
            " WHERE"
            "   id = :id"
        );
   
        ret.affected_rows = st.set("id", id).
                               execute();
        ret.rowid = conn.lastInsertId();
        log_debug ("[t_bios_alert]: was deleted %" 
                                        PRIu64 " rows", ret.affected_rows);
        ret.status = 1;
        LOG_END;
        return ret;
    }
    catch (const std::exception &e) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = e.what();
        LOG_END_ABNORMAL(e);
        return ret;
    }
}


//=============================================================================
db_reply_t
    insert_into_alert_devices 
        (tntdb::Connection  &conn,
         m_alrt_id_t               alert_id,
         std::vector <std::string> device_names)
{
    LOG_START;
    log_debug ("  alert_id = %" PRIi32, alert_id);
    log_debug ("  devices count = %zu", device_names.size());

    db_reply_t ret = db_reply_new();

    // input parameters control
    if ( alert_id == 0 )
    {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "0 value of alert_id is not allowed";
        log_error ("end: %s, %s", "ignore insert", ret.msg);
        return ret;
    }
    if ( device_names.size() == 0 )
    {
        ret.status  = 1;
        log_info ("end: %s, %s","ignore insert", "noting to insert");
        return ret;
    }
    // actually, if there is nothing to insert, then insert was ok :)
    log_debug ("input parameters are correct");

    for (auto &device_name : device_names )
    {
        auto reply_internal = insert_into_alert_device
                                (conn, alert_id, device_name.c_str());
        if ( reply_internal.status == 1 )
            ret.affected_rows++;
    }

    if ( ret.affected_rows == device_names.size() )
    {
        ret.status = 1;
        log_debug ("all ext attributes were inserted successfully");
        LOG_END;
        return ret;
    }
    else
    {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "not all attributes were inserted";
        log_error ("end: %s", ret.msg);
        return ret;
    }
}

db_reply_t
    insert_into_alert_device
        (tntdb::Connection &conn,
         m_alrt_id_t        alert_id,
         const char        *device_name)
{
    LOG_START;
    log_debug ("  alert_id = %" PRIi32, alert_id);
    log_debug ("  device_name = '%s'", device_name);

    db_reply_t ret = db_reply_new();

    // input parameters control
    if ( alert_id == 0 )
    {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "0 value of alert_id is not allowed";
        log_error ("end: %s, %s", "ignore insert", ret.msg);
        return ret;
    }
    if ( !is_ok_name(device_name) )
    {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "value of device_name is invalid";
        log_error ("end: %s, %s", "ignore insert", ret.msg);
        return ret;
    }
    log_debug ("input parameters are correct");

    try{
        tntdb::Statement st = conn.prepareCached(
            " INSERT INTO"
            "   t_bios_alert_device"
            "   (alert_id, device_id)"
            " SELECT"
            "   :alert, v.id_discovered_device"
            " FROM"
            "   t_bios_monitor_asset_relation v,"
            "   t_bios_asset_element v1"
            " WHERE"
            "   v.id_asset_element = v1.id_asset_element AND"
            "   v1.name = :name"
        );
   
        ret.affected_rows = st.set("alert", alert_id).
                               set("name", device_name).
                               execute();
        ret.rowid = conn.lastInsertId();
        log_debug ("[t_bios_alert_device]: was inserted %" 
                                    PRIu64 " rows", ret.affected_rows);
        ret.status = 1;
        LOG_END;
        return ret;
    }
    catch (const std::exception &e) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = e.what();
        LOG_END_ABNORMAL(e);
        return ret;
    }
}

//=============================================================================
db_reply_t
    delete_from_alert_device
        (tntdb::Connection &conn,
         m_alrtdvc_id_t    id)
{
    LOG_START;
    log_debug ("  id = %" PRIu32, id);

    db_reply_t ret = db_reply_new();

    try{
        tntdb::Statement st = conn.prepareCached(
            " DELETE FROM"
            "   t_bios_alert_device"
            " WHERE"
            "   id = :id"
        );
   
        ret.affected_rows = st.set("id", id).
                               execute();
        ret.rowid = conn.lastInsertId();
        log_debug ("[t_bios_alert_device]: was deleted %" 
                                        PRIu64 " rows", ret.affected_rows);
        ret.status = 1;
        LOG_END;
        return ret;
    }
    catch (const std::exception &e) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = e.what();
        LOG_END_ABNORMAL(e);
        return ret;
    }
}


//=============================================================================
db_reply_t
    insert_new_alert 
        (tntdb::Connection  &conn,
         const char         *rule_name,
         a_elmnt_pr_t        priority,
         m_alrt_state_t      alert_state,
         const char         *description,
         m_alrt_ntfctn_t     notification,
         int64_t             date_from,
         std::vector<std::string> device_names)
{
    LOG_START;
 
    tntdb::Transaction trans(conn);
    auto reply_internal1 = insert_into_alert
        (conn, rule_name, priority, alert_state, description,
         notification, date_from);
    if ( ( reply_internal1.status == 0 ) ||
         ( reply_internal1.affected_rows == 0 ) )
    {
        trans.rollback();
        log_info ("end: alarm was not inserted (fail in alert");
        return reply_internal1;
    }
    auto alert_id = reply_internal1.rowid;

    auto reply_internal2 = insert_into_alert_devices
                                    (conn, alert_id, device_names);
    if ( ( reply_internal2.status == 0 ) ||
         ( reply_internal2.affected_rows == 0 ) )
    {
        trans.rollback();
        log_info ("end: alarm was not inserted (fail in alert devices");
        return reply_internal2;
    }
    trans.commit();
    LOG_END;
    return reply_internal1;
}


//=============================================================================
static const std::string  sel_alarm_opened_QUERY =
    " SELECT"
    "    v.id, v.rule_name, v.priority, v.state,"
    "    v.descriprion, v.notification,"
    "    UNIX_TIMESTAMP(v.date_from), UNIX_TIMESTAMP(v.date_till)"
    " FROM"
    "   v_bios_alert v"
    " WHERE v.date_till is NULL";

static const std::string  sel_alarm_closed_QUERY =
    " SELECT"
    "    v.id, v.rule_name, v.priority, v.state,"
    "    v.descriprion, v.notification,"
    "    UNIX_TIMESTAMP(v.date_from), UNIX_TIMESTAMP(v.date_till)"
    " FROM"
    "   v_bios_alert v"
    " WHERE v.date_till is not NULL";

static db_reply <std::vector<db_alert_t>>
    select_alert_all_template
        (tntdb::Connection  &conn,
         const std::string  &query)
{   
    LOG_START;
    std::vector<db_alert_t> item{};
    db_reply<std::vector<db_alert_t>> ret = db_reply_new(item);

    try {
        tntdb::Statement st = conn.prepareCached(query);
        tntdb::Result res = st.select();
        
        log_debug ("[t_bios_alert]: was %u rows selected", res.size());

        for ( auto &r : res ) {
            std::vector<m_dvc_id_t> dvc_ids{}; 
            db_alert_t m = {0, "", 0, 0, "", 0 , 0, 0, dvc_ids};

            r[0].get(m.id);
            r[1].get(m.rule_name);
            r[2].get(m.priority);
            r[3].get(m.alert_state);
            r[4].get(m.description);
            r[5].get(m.notification);
            r[6].get(m.date_from);
            r[7].get(m.date_till);
            
            auto reply_internal = select_alert_devices (conn, m.id);
            if ( reply_internal.status == 0 )
            {
                ret.status     = 0;
                ret.errtype    = DB_ERR;
                ret.errsubtype = DB_ERROR_BADINPUT; // TODO ERROR
                ret.msg        = "error in device selecting";
                ret.item.clear();
                log_error ("end: %s, %s", "ignore select", ret.msg);
                return ret;
            }
            ret.item.push_back(m);
        }
        ret.status = 1;
    } catch(const std::exception &e) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = e.what();
        ret.item.clear();
        LOG_END_ABNORMAL(e);
        return ret;
    }
    LOG_END;
    return ret;
}


//=============================================================================
db_reply <std::vector<db_alert_t>>
    select_alert_all_opened
        (tntdb::Connection  &conn)
{
    return select_alert_all_template(conn, sel_alarm_opened_QUERY);
}


//=============================================================================
db_reply <std::vector<db_alert_t>>
    select_alert_all_closed
        (tntdb::Connection  &conn)
{
    return select_alert_all_template(conn, sel_alarm_closed_QUERY);
}


//=============================================================================
db_reply <std::vector<m_dvc_id_t>>
    select_alert_devices
        (tntdb::Connection &conn,
         m_alrt_id_t        alert_id)
{   
    LOG_START;
    std::vector<m_dvc_id_t> item{};
    db_reply<std::vector<m_dvc_id_t>> ret = db_reply_new(item);

    try {
        tntdb::Statement st = conn.prepareCached(
                " SELECT"
                "    v.device_id"
                " FROM"
                "   v_bios_alert_device v"
                " WHERE v.alert_id = :alert"
        );
        tntdb::Result res = st.set("alert", alert_id).
                               select();
        
        log_debug ("[t_bios_alert_device]: was %u rows selected", res.size());

        for ( auto &r : res ) {
            m_dvc_id_t device_id = 0;
            r[0].get(device_id);
            ret.item.push_back(device_id);
        }
        ret.status = 1;
    } catch(const std::exception &e) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = e.what();
        ret.item.clear();
        LOG_END_ABNORMAL(e);
        return ret;
    }
    LOG_END;
    return ret;
}