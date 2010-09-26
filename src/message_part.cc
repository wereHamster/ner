/* ner: src/message_part.cc
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

#include "message_part.hh"
#include "ner_config.hh"
#include "gmime_iostream.hh"
#include "message_part_visitor.hh"

MessagePart::MessagePart(const std::string & id_)
    : id(id_)
{
}

TextPart::TextPart(GMimePart * part)
    : MessagePart(g_mime_part_get_content_id(part) ? : std::string())
{
    GMimeContentType * contentType = g_mime_object_get_content_type(GMIME_OBJECT(part));

    GMimeStream * contentStream = NULL;

    /* If this part is text */
    else if (g_mime_content_type_is_type(contentType, "text", "*"))
    {
        GMimeDataWrapper * content = g_mime_part_get_content_object(part);
        const char * charset = g_mime_object_get_content_type_parameter(GMIME_OBJECT(part), "charset");
        GMimeStream * stream = g_mime_data_wrapper_get_stream(content);

        GMimeStream * filteredStream = g_mime_stream_filter_new(stream);

        GMimeFilter * filter = g_mime_filter_basic_new(g_mime_data_wrapper_get_encoding(content), false);
        g_mime_stream_filter_add(GMIME_STREAM_FILTER(filteredStream), filter);
        g_object_unref(filter);

        if (charset)
        {
            GMimeFilter * filter = g_mime_filter_charset_new(charset, "UTF-8");
            g_mime_stream_filter_add(GMIME_STREAM_FILTER(filteredStream), filter);
            g_object_unref(filter);
        }

        g_mime_stream_reset(stream);

        contentStream = filteredStream;
    }
    else
    {
        /* We don't know how to handle this part */
    }

    if (contentStream == NULL)
        throw std::runtime_error(std::string("Cannot handle content type: ") +
            g_mime_content_type_to_string(contentType));

    GMimeIOStream stream(contentStream);
    g_object_unref(contentStream);

    while (stream.good())
    {
        std::string line;
        std::getline(stream, line);
        for (std::size_t tab = 0; (tab = line.find('\t', tab)) != std::string::npos; ++tab)
            line.replace(tab, 1, 8 - (tab % 8), ' ');
        lines.push_back(line);
    }
}

void TextPart::accept(MessagePartVisitor & visitor)
{
    visitor.visit(*this);
}

Attachment::Attachment(GMimePart * part)
    : MessagePart(g_mime_part_get_content_id(part) ? : std::string()),
        filename(g_mime_part_get_filename(part) ? : std::string()),
        data(g_mime_part_get_content_object(part))
{
    filesize = g_mime_stream_length(g_mime_data_wrapper_get_stream(data));
}

void Attachment::accept(MessagePartVisitor & visitor)
{
    visitor.visit(*this);
}

// vim: fdm=syntax fo=croql et sw=4 sts=4 ts=8

