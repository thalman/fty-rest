/*
 #
 # Copyright (C) 2015-2016 Eaton
 #
 # This program is free software; you can redistribute it and/or modify
 # it under the terms of the GNU General Public License as published by
 # the Free Software Foundation; either version 2 of the License, or
 # (at your option) any later version.
 #
 # This program is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License along
 # with this program; if not, write to the Free Software Foundation, Inc.,
 # 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 #
 #
*/
/*!
 * \file warranty-metric.cc
 * \author Michal Vyskocil <MichalVyskocil@Eaton.com>
 * \brief  Send a warranty metric for all elements with element_date in the DB
 */

#include <malamute.h>
#include <fty_proto.h>
#include <tntdb.h>

#include "log.h"
#include "dbpath.h"
#include "db/assets.h"

/*
 *  HOTFIX: let it generate alert itself, pattern rules in alert-generator are broken!
 *
 */

/*
 * Tool will send following messages on the stream METRICS
 *
 *  SUBJECT: end_warranty_date@device
 *           value now() - end_warranty_date
 */
uint32_t TTL = 24*60*60;//[s]
int main()
{
    log_open ();

    mlm_client_t *client = mlm_client_new ();
    assert (client);

    std::function<void(const tntdb::Row&)> cb = \
        [client](const tntdb::Row &row)
        {
            std::string name;
            row["name"].get(name);

            std::string keytag;
            row["keytag"].get(keytag);

            std::string date;
            row["date"].get(date);
            
            int day_diff;
            {
                struct tm tm_ewd;
                ::memset (&tm_ewd, 0, sizeof(struct tm));

                char* ret = ::strptime (date.c_str(), "%Y-%m-%d", &tm_ewd);
                if (ret == NULL) {
                    log_error ("Cannot convert %s to date, skipping", date.c_str());
                    return;
                }

                time_t ewd = ::mktime (&tm_ewd);
                time_t now = ::time (NULL);

                struct tm *tm_now_p;
                tm_now_p = ::gmtime (&now);
                tm_now_p->tm_hour = 0;
                tm_now_p->tm_min = 0;
                tm_now_p->tm_sec = 0;
                now = ::mktime (tm_now_p);

                // end_warranty_date (s) - now (s) -> to days
                day_diff = std::ceil ((ewd - now) / (60*60*24));
                log_debug ("day_diff: %d", day_diff);
            }
            log_debug ("name: %s, keytag: %s, date: %s", name.c_str(), keytag.c_str(), date.c_str());
            zmsg_t *msg = fty_proto_encode_metric (
                    NULL,
                    ::time (NULL),
                    3 * TTL,
                    keytag.c_str(),
                    name.c_str (),
                    std::to_string (day_diff).c_str(),
                    "day");
            assert (msg);
            std::string subject = keytag.append ("@").append (name);
            mlm_client_send (client, subject.c_str (), &msg);
        };

    int r = mlm_client_connect (client, "ipc://@/malamute", 1000, "warranty-metric");
    if (r == -1) {
        log_error ("Can't connect to malamute");
        exit (EXIT_FAILURE);
    }

    r = mlm_client_set_producer (client, "METRICS");
    if (r == -1) {
        log_error ("Can't set producer to METRICS stream");
        exit (EXIT_FAILURE);
    }

    // unchecked errors with connection, the tool will fail otherwise
    tntdb::Connection conn = tntdb::connectCached(url);
    r = persist::select_asset_element_all_with_warranty_end (conn, cb);
    if (r == -1) {
        log_error ("Error in element selection");
        exit (EXIT_FAILURE);
    }

    // to ensure all messages got published
    zclock_sleep (500);
    mlm_client_destroy (&client);

    exit (EXIT_SUCCESS);
}
