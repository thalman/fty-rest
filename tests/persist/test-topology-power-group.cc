/*
 *
 * Copyright (C) 2015 Eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

/*!
 * \file test-topology-power-group.cc
 * \author Alena Chernikava <AlenaChernikava@Eaton.com>
 * \author Karol Hrdina <KarolHrdina@Eaton.com>
 * \author Michal Hrusecky <MichalHrusecky@Eaton.com>
 * \brief Not yet documented file
 */
#include <catch.hpp>

#include <iostream>
#include <czmq.h>

#include "dbpath.h"
#include "log.h"

#include "assettopology.h"
#include "common_msg.h"
#include "assetcrud.h"

#include "cleanup.h"

TEST_CASE("Power topology group #1","[db][topology][power][group][power_topology.sql][pg1]")
{
    log_open();

    log_info ("=============== POWER GROUP #1 ==================");
    _scoped_asset_msg_t* getmsg = asset_msg_new (ASSET_MSG_GET_POWER_GROUP);
    assert ( getmsg );
    asset_msg_set_element_id (getmsg, 5088);
//    asset_msg_print (getmsg);

    // the expected devices
    std::set<std::tuple<int,std::string,std::string>> sdevices;
    sdevices.insert (std::make_tuple(5087, "MAIN-05", "feed")); // id,  device_name, device_type_name

    _scoped_zmsg_t* retTopology = get_return_power_topology_group (url.c_str(), getmsg);
    assert ( retTopology );
    REQUIRE ( is_asset_msg (retTopology) );
    _scoped_asset_msg_t* cretTopology = asset_msg_decode (&retTopology);
    assert ( cretTopology );
//    asset_msg_print (cretTopology);
//    print_frame_devices (asset_msg_devices (cretTopology));

    // check powers
    _scoped_zlist_t* powers = asset_msg_get_powers (cretTopology);
    REQUIRE ( zlist_size(powers) == 0 );
    zlist_destroy (&powers);

    // check the devices
    zframe_t* frame = asset_msg_devices (cretTopology);

#if CZMQ_VERSION_MAJOR == 3
    byte* buffer = zframe_data (frame);
    assert ( buffer );
    
    _scoped_zmsg_t *zmsg = zmsg_decode ( buffer, zframe_size (frame));
#else
    _scoped_zmsg_t *zmsg = zmsg_decode (frame);
#endif
    assert ( zmsg );
    assert ( zmsg_is (zmsg) );

    _scoped_zmsg_t* pop = NULL;
    int n = sdevices.size();
    for (int i = 1 ; i <= n ; i ++ )
    {
        pop = zmsg_popmsg (zmsg);
        INFO(n);
        REQUIRE ( pop != NULL );

        _scoped_asset_msg_t* item = asset_msg_decode (&pop); // pop is freed
        assert ( item );
//    asset_msg_print (item);
        auto it = sdevices.find ( std::make_tuple ( asset_msg_element_id (item),
                                                    asset_msg_name (item),
                                                    asset_msg_type_name (item) ));
        REQUIRE ( it != sdevices.end() );
        sdevices.erase (it);
        asset_msg_destroy (&item);
    }

    // there is no more devices
    pop = zmsg_popmsg (zmsg);
    REQUIRE ( pop == NULL );
    REQUIRE ( sdevices.empty() );

    asset_msg_destroy (&getmsg);
    asset_msg_destroy (&cretTopology);
}
