/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITYCORE_TAXI_PACKETS_H
#define TRINITYCORE_TAXI_PACKETS_H

#include "Packet.h"
#include "ObjectGuid.h"
#include "DBCEnums.h"
#include "Optional.h"

namespace WorldPackets
{
    namespace Taxi
    {
        class TaxiNodeStatusQuery final : public ClientPacket
        {
        public:
            explicit TaxiNodeStatusQuery(WorldPacket&& packet) : ClientPacket(CMSG_TAXI_NODE_STATUS_QUERY, std::move(packet)) { }

            void Read() override;

            ObjectGuid UnitGUID;
        };

        class TaxiNodeStatus final : public ServerPacket
        {
        public:
            explicit TaxiNodeStatus() : ServerPacket(SMSG_TAXI_NODE_STATUS, 16 + 1) { }

            WorldPacket const* Write() override;

            uint8 Status = 0; // replace with TaxiStatus enum
            ObjectGuid Unit;
        };

        struct ShowTaxiNodesWindowInfo
        {
            ObjectGuid UnitGUID;
            int32 CurrentNode = 0;
        };

        class ShowTaxiNodes final : public ServerPacket
        {
        public:
            explicit ShowTaxiNodes() : ServerPacket(SMSG_SHOW_TAXI_NODES) { }

            WorldPacket const* Write() override;

            Optional<ShowTaxiNodesWindowInfo> WindowInfo;
            TaxiMask CanLandNodes; // Nodes known by player
            TaxiMask CanUseNodes;  // Nodes available for use - this can temporarily disable a known node
        };

        class EnableTaxiNode final : public ClientPacket
        {
        public:
            explicit EnableTaxiNode(WorldPacket&& packet) : ClientPacket(CMSG_ENABLE_TAXI_NODE, std::move(packet)) { }

            void Read() override;

            ObjectGuid Unit;
        };

        class TaxiQueryAvailableNodes final : public ClientPacket
        {
        public:
            explicit TaxiQueryAvailableNodes(WorldPacket&& packet) : ClientPacket(CMSG_TAXI_QUERY_AVAILABLE_NODES, std::move(packet)) { }

            void Read() override;

            ObjectGuid Unit;
        };

        class ActivateTaxi final : public ClientPacket
        {
        public:
            explicit ActivateTaxi(WorldPacket&& packet) : ClientPacket(CMSG_ACTIVATE_TAXI, std::move(packet)) { }

            void Read() override;

            ObjectGuid Vendor;
            uint32 Node = 0;
            uint32 GroundMountID = 0;
            uint32 FlyingMountID = 0;
        };

        class NewTaxiPath final : public ServerPacket
        {
        public:
            explicit NewTaxiPath(int32 taxiNodesId) : ServerPacket(SMSG_NEW_TAXI_PATH, 4), TaxiNodesID(taxiNodesId) { }

            WorldPacket const* Write() override;

            int32 TaxiNodesID = 0;
        };

        class ActivateTaxiReply final : public ServerPacket
        {
        public:
            explicit ActivateTaxiReply() : ServerPacket(SMSG_ACTIVATE_TAXI_REPLY, 1) { }

            WorldPacket const* Write() override;

            uint8 Reply = 0;
        };

        class TaxiRequestEarlyLanding final : public ClientPacket
        {
        public:
            explicit TaxiRequestEarlyLanding(WorldPacket&& packet) : ClientPacket(CMSG_TAXI_REQUEST_EARLY_LANDING, std::move(packet)) { }

            void Read() override { }
        };
    }
}

#endif // TRINITYCORE_TAXI_PACKETS_H
