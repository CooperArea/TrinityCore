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

#ifndef TRINITYCORE_CHAT_H
#define TRINITYCORE_CHAT_H

#include "ObjectGuid.h"
#include "StringFormat.h"
#include <fmt/printf.h>

class ChatHandler;
class Creature;
class GameObject;
class Group;
class Player;
class Unit;
class WorldSession;
class WorldObject;

struct GameTele;

enum LocaleConstant : uint8;

class TC_GAME_API ChatHandler
{
    public:
        bool IsConsole() const { return (m_session == nullptr); }
        WorldSession* GetSession() { return m_session; }
        WorldSession const* GetSession() const { return m_session; }
        Player* GetPlayer() const;
        explicit ChatHandler(WorldSession* session) : m_session(session), sentErrorMessage(false) { }
        ChatHandler(ChatHandler const&) = delete;
        ChatHandler(ChatHandler&&) = delete;
        ChatHandler& operator=(ChatHandler const&) = delete;
        ChatHandler& operator=(ChatHandler&&) = delete;
        virtual ~ChatHandler() = default;

        static char* LineFromMessage(char*& pos);

        // function with different implementation for chat/console
        virtual char const* GetTrinityString(uint32 entry) const;
        virtual void SendSysMessage(std::string_view str, bool escapeCharacters = false);

        void SendSysMessage(uint32 entry);

        template<typename... Args>
        void PSendSysMessage(char const* fmt, Args&&... args)
        {
            SendSysMessage(StringVPrintf(fmt, fmt::make_printf_args(std::forward<Args>(args)...)));
        }

        template<typename... Args>
        void PSendSysMessage(uint32 entry, Args&&... args)
        {
            SendSysMessage(PGetParseString(entry, std::forward<Args>(args)...));
        }

        template<typename... Args>
        static std::string PGetParseString(std::string_view fmt, Args&&... args)
        {
            return StringVPrintf(fmt, fmt::make_printf_args(std::forward<Args>(args)...));
        }

        template<typename... Args>
        std::string PGetParseString(uint32 entry, Args&&... args) const
        {
            return PGetParseString(GetTrinityString(entry), std::forward<Args>(args)...);
        }

        static std::string StringVPrintf(std::string_view messageFormat, fmt::printf_args messageFormatArgs);

        bool _ParseCommands(std::string_view text);
        virtual bool ParseCommands(std::string_view text);

        void SendGlobalSysMessage(const char *str);

        // function with different implementation for chat/console
        virtual bool IsHumanReadable() const { return true; }
        virtual bool HasPermission(uint32 permission) const;
        virtual std::string GetNameLink() const;
        virtual bool needReportToTarget(Player* chr) const;
        virtual LocaleConstant GetSessionDbcLocale() const;
        virtual LocaleConstant GetSessionDbLocaleIndex() const;

        bool HasLowerSecurity(Player* target, ObjectGuid guid, bool strong = false);
        bool HasLowerSecurityAccount(WorldSession* target, uint32 account, bool strong = false);

        void SendGlobalGMSysMessage(const char *str);
        Player* getSelectedPlayer();
        Creature* getSelectedCreature();
        Unit* getSelectedUnit();
        WorldObject* getSelectedObject();
        // Returns either the selected player or self if there is no selected player
        Player* getSelectedPlayerOrSelf();

        char* extractKeyFromLink(char* text, char const* linkType, char** something1 = nullptr);
        char* extractKeyFromLink(char* text, char const* const* linkTypes, int* found_idx, char** something1 = nullptr);
        char* extractQuotedArg(char* args);
        ObjectGuid::LowType extractLowGuidFromLink(char* text, HighGuid& guidHigh);
        bool GetPlayerGroupAndGUIDByName(const char* cname, Player*& player, Group*& group, ObjectGuid& guid, bool offline = false);
        std::string extractPlayerNameFromLink(char* text);
        // select by arg (name/link) or in-game selection online/offline player or self if a creature is selected
        bool extractPlayerTarget(char* args, Player** player, ObjectGuid* player_guid = nullptr, std::string* player_name = nullptr);

        std::string playerLink(std::string const& name) const;
        std::string GetNameLink(Player* chr) const;

        GameObject* GetNearbyGameObject();
        GameObject* GetObjectFromPlayerMapByDbGuid(ObjectGuid::LowType lowguid);
        Creature* GetCreatureFromPlayerMapByDbGuid(ObjectGuid::LowType lowguid);
        bool HasSentErrorMessage() const { return sentErrorMessage; }
        void SetSentErrorMessage(bool val){ sentErrorMessage = val; }
    protected:
        explicit ChatHandler() : m_session(nullptr), sentErrorMessage(false) { }     // for CLI subclass

    private:
        WorldSession* m_session;                           // != nullptr for chat command call and nullptr for CLI command

        // common global flag
        bool sentErrorMessage;
};

class TC_GAME_API CliHandler : public ChatHandler
{
    public:
        using Print = void(void*, std::string_view);
        explicit CliHandler(void* callbackArg, Print* zprint) : m_callbackArg(callbackArg), m_print(zprint) { }

        // overwrite functions
        char const* GetTrinityString(uint32 entry) const override;
        bool HasPermission(uint32 /*permission*/) const override { return true; }
        void SendSysMessage(std::string_view, bool escapeCharacters) override;
        bool ParseCommands(std::string_view str) override;
        std::string GetNameLink() const override;
        bool needReportToTarget(Player* chr) const override;
        LocaleConstant GetSessionDbcLocale() const override;
        LocaleConstant GetSessionDbLocaleIndex() const override;

    private:
        void* m_callbackArg;
        Print* m_print;
};

class TC_GAME_API AddonChannelCommandHandler : public ChatHandler
{
    public:
        static std::string const PREFIX;

        using ChatHandler::ChatHandler;
        bool ParseCommands(std::string_view str) override;
        void SendSysMessage(std::string_view, bool escapeCharacters) override;
        using ChatHandler::SendSysMessage;
        bool IsHumanReadable() const override { return humanReadable; }

    private:
        void Send(std::string const& msg);
        void SendAck();
        void SendOK();
        void SendFailed();

        char const* echo = nullptr;
        bool hadAck = false;
        bool humanReadable = false;
};

#endif
