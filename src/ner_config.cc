/* ner: src/ner_config.cc
 *
 * Copyright (c) 2010, 2012 Michael Forney
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

#include <fstream>
#include <yaml-cpp/yaml.h>

#include "ner_config.hh"
#include "identity_manager.hh"
#include "colors.hh"
#include "ncurses.hh"

const std::string nerConfigFile(".ner.yaml");

namespace YAML {
    template<>
    struct convert<Color> {
        static bool decode(const Node & node, Color & color) {
            static const std::map<std::string, int> ncursesColors = {
                { "black",      COLOR_BLACK },
                { "red",        COLOR_RED },
                { "green",      COLOR_GREEN },
                { "yellow",     COLOR_YELLOW },
                { "blue",       COLOR_BLUE },
                { "magenta",    COLOR_MAGENTA },
                { "cyan",       COLOR_CYAN },
                { "white",      COLOR_WHITE }
            };

            std::string foreground = node["fg"].as<std::string>();
            std::string background = node["bg"].as<std::string>();

            color.foreground = ncursesColors.at(foreground);
            color.background = ncursesColors.at(background);

            return true;
        }
    };

    template <>
    struct convert<Search> {
        static bool decode(const Node & node, Search & search) {
            search.name  = node["name"].as<std::string>();
            search.query = node["query"].as<std::string>();

            return true;
        }
    };
}


NerConfig & NerConfig::instance()
{
    static NerConfig * config = NULL;

    if (!config)
        config = new NerConfig();

    return *config;
}

NerConfig::NerConfig()
{
}

NerConfig::~NerConfig()
{
}

void NerConfig::load()
{
    _sortMode = NOTMUCH_SORT_NEWEST_FIRST;
    _refreshView = true;
    _addSigDashes = true;
    _commands.clear();

    std::map<ColorID, Color> colorMap = defaultColorMap;

    std::string configPath(std::string(getenv("HOME")) + "/" + nerConfigFile);
    YAML::Node document = YAML::LoadFile(configPath.c_str());

    if (document.Type() != YAML::NodeType::Null)
    {
        /* Identities */
        IdentityManager::instance().load(document["identities"]);

        auto defaultIdentity = document["default_identity"];
        if (defaultIdentity.IsDefined())
            IdentityManager::instance().setDefaultIdentity(defaultIdentity.as<std::string>());

        /* General stuff */
        auto general = document["general"];
        if (general.IsDefined())
        {
            auto sortModeNode = general["sort_mode"];
            if (sortModeNode.IsDefined())
            {
                std::string sortModeStr = sortModeNode.as<std::string>();

                if (sortModeStr == std::string("oldest_first"))
                    _sortMode = NOTMUCH_SORT_OLDEST_FIRST;
                else if (sortModeStr == std::string("message_id"))
                    _sortMode = NOTMUCH_SORT_MESSAGE_ID;
                else if (sortModeStr != std::string("newest_first"))
                {
                    /* FIXME: throw? */
                }
            }

            auto refreshViewNode = general["refresh_view"];
            if (refreshViewNode.IsDefined())
                _refreshView = refreshViewNode.as<bool>();

            auto addSigDashesNode = general["add_sig_dashes"];
            if (addSigDashesNode.IsDefined())
                _addSigDashes = addSigDashesNode.as<bool>();
        }

        /* Commands */
        const YAML::Node commands = document["commands"];
        if (commands.IsDefined())
            _commands = commands.as<std::map<std::string, std::string>>();

        /* Saved Searches */
        const YAML::Node searches = document["searches"];
        if (searches.IsDefined())
            _searches = searches.as<std::vector<Search>>();
        else
            _searches = {
                { "New", "tag:inbox and tag:unread" },
                { "Unread", "tag:unread" },
                { "Inbox", "tag:inbox" }
            };

        /* Colors */
        const YAML::Node colors = document["colors"];
        if (colors.IsDefined())
        {
            std::map<std::string, ColorID> colorNames = {
                /* General */
                { "cut_off_indicator",      ColorID::CutOffIndicator },
                { "more_less_indicator",    ColorID::MoreLessIndicator },
                { "empty_space_indicator",  ColorID::EmptySpaceIndicator },
                { "line_wrap_indicator",    ColorID::LineWrapIndicator },

                /* Status Bar */
                { "status_bar_status",          ColorID::StatusBarStatus },
                { "status_bar_status_divider",  ColorID::StatusBarStatusDivider },
                { "status_bar_message",         ColorID::StatusBarMessage },
                { "status_bar_prompt",          ColorID::StatusBarPrompt },

                /* Search View */
                { "search_view_date",                   ColorID::SearchViewDate },
                { "search_view_message_count_complete", ColorID::SearchViewMessageCountComplete },
                { "search_view_message_count_partial",  ColorID::SearchViewMessageCountPartial },
                { "search_view_authors",                ColorID::SearchViewAuthors },
                { "search_view_subject",                ColorID::SearchViewSubject },
                { "search_view_tags",                   ColorID::SearchViewTags },

                /* Thread View */
                { "thread_view_arrow",  ColorID::ThreadViewArrow },
                { "thread_view_date",   ColorID::ThreadViewDate },
                { "thread_view_tags",   ColorID::ThreadViewTags },

                /* Email View */
                { "email_view_header",  ColorID::EmailViewHeader },

                /* View View */
                { "view_view_number",   ColorID::ViewViewNumber },
                { "view_view_name",     ColorID::ViewViewName },
                { "view_view_status",   ColorID::ViewViewStatus },

                /* Search List View */
                { "search_list_view_name",      ColorID::SearchListViewName },
                { "search_list_view_terms",     ColorID::SearchListViewTerms },
                { "search_list_view_results",   ColorID::SearchListViewTerms },

                /* Message Parts */
                { "attachment_filename",        ColorID::AttachmentFilename },
                { "attachment_mimetype",        ColorID::AttachmentMimeType },
                { "attachment_filesize",        ColorID::AttachmentFilesize },

                /* Citation levels */
                { "citation_level_1",           ColorID::CitationLevel1 },
                { "citation_level_2",           ColorID::CitationLevel2 },
                { "citation_level_3",           ColorID::CitationLevel3 },
                { "citation_level_4",           ColorID::CitationLevel4 }
            };

            for (auto name = colors.begin(), e = colors.end(); name != e; ++name)
                colorMap[colorNames.at(name->first.as<std::string>())] = name->second.as<Color>();
        }
    }

    /* Initialize colors from color map. */
    for (auto color = colorMap.begin(), e = colorMap.end(); color != e; ++color)
        init_pair(color->first, color->second.foreground, color->second.background);
}

std::string NerConfig::command(const std::string & name)
{
    auto command = _commands.find(name);

    if (command == _commands.end())
    {
        /* Use a default */
        if (name == "send")
            return "/usr/sbin/sendmail -t";
        else if (name == "edit")
            return "vim +";
    }
    else
    {
        return command->second;
    }
}

const std::vector<Search> & NerConfig::searches() const
{
    return _searches;
}

notmuch_sort_t NerConfig::sortMode() const
{
    return _sortMode;
}

bool NerConfig::refreshView() const
{
    return _refreshView;
}

bool NerConfig::addSigDashes() const
{
    return _addSigDashes;
}

// vim: fdm=syntax fo=croql et sw=4 sts=4 ts=8

