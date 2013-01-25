/* ner: src/identity_manager.cc
 *
 * Copyright (c) 2010 Michael Forney
 *
 * This file is a part of ner.
 *
 * ner is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 3, as published by the Free
 * Software Foundation.
 *
 * ner is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ner.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "identity_manager.hh"

#include "maildir.hh"
#include "notmuch.hh"


namespace YAML {
    template<>
    struct convert<Identity> {
        static bool decode(const Node & node, Identity & identity) {
            identity.name  = node["name"].as<std::string>();
            identity.email = node["email"].as<std::string>();

            /* Optional entries */
            const YAML::Node signatureNode = node["signature"];
            const YAML::Node sendCopyToSelfNode = node["bcc"];
            const YAML::Node sendNode = node["send"];
            const YAML::Node sentMailNode = node["sent_mail"];

            if (signatureNode)
                identity.signaturePath = signatureNode.as<std::string>();
            else
                identity.signaturePath.clear();

            if (sendCopyToSelfNode.IsDefined())
                identity.sendCopyToSelf = sendCopyToSelfNode.as<bool>();

            if (sendNode.IsDefined())
                identity.sendCommand = sendNode.as<std::string>();

            std::string tagPrefix = "tag:the-ner.org,2010:";

            identity.sentMail.reset();

            if (sentMailNode.IsDefined())
            {
                if (sentMailNode.Tag() == tagPrefix + "maildir")
                {
                    std::string sentMailPath = sentMailNode.as<std::string>();
                    identity.sentMail = std::make_shared<Maildir>(sentMailPath);
                }
            }

            return true;
        }
    };
}

IdentityManager & IdentityManager::instance()
{
    static IdentityManager * manager = NULL;

    if (!manager)
        manager = new IdentityManager();

    return *manager;
}

IdentityManager::IdentityManager()
{
}

IdentityManager::~IdentityManager()
{
}

void IdentityManager::load(const YAML::Node node)
{
    _identities.clear();

    if (node.IsDefined())
        _identities = node.as<std::map<std::string, Identity>>();
    /* Otherwise, guess identities from notmuch config */
    else
    {
        Identity identity;
        identity.name = g_key_file_get_string(Notmuch::config(), "user", "name", NULL);

        identity.email = g_key_file_get_string(Notmuch::config(), "user", "primary_email", NULL);
        _identities.insert(std::make_pair(identity.email, identity));

        size_t addressesLength;
        char ** addresses = g_key_file_get_string_list(Notmuch::config(), "user", "other_email",
            &addressesLength, NULL);

        for (int index = 0; index < addressesLength; ++index)
        {
            identity.email = addresses[index];
            _identities.insert(std::make_pair(identity.email, identity));
        }
    }
}

void IdentityManager::setDefaultIdentity(const std::string & identity)
{
    _defaultIdentity = identity;
}

const Identity * IdentityManager::defaultIdentity() const
{
    auto identity = _identities.find(_defaultIdentity);

    if (identity == _identities.end())
        /* We couldn't find it, just use the first one */
        identity = _identities.begin();

    return &identity->second;
}

const Identity * IdentityManager::findIdentity(InternetAddress * address)
{
    std::string email = internet_address_mailbox_get_addr(INTERNET_ADDRESS_MAILBOX(address));

    for (auto identity = _identities.begin(); identity != _identities.end(); ++identity)
    {
        if (identity->second.email == email)
            return &identity->second;
    }

    return 0;
}

const Identity * IdentityManager::findIdentity(const std::string & name)
{
    auto identity = _identities.find(name);

    if (identity != _identities.end())
        return &identity->second;

    return 0;
}

// vim: fdm=syntax fo=croql et sw=4 sts=4 ts=8

