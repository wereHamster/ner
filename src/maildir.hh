/* ner: src/maildir.hh
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

#ifndef NER_MAILDIR_H
#define NER_MAILDIR_H 1

#include "mail_store.hh"

class Maildir : public MailStore
{
    public:
        Maildir(const std::string & path);
        virtual ~Maildir();

        virtual bool addMessage(GMimeMessage * message);

    private:
        static int deliveries;

        std::string _path;
};

#endif

// vim: fdm=syntax fo=croql et sw=4 sts=4 ts=8

