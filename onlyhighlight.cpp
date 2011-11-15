#include <main.h>
#include <Modules.h>
#include <znc/User.h>
#include <znc/Chan.h>

#include <map>

#define DEFAULT_LINES_TO_BUFFER 5


class OnlyHighlightMod : public CModule {

public:

    MODCONSTRUCTOR(OnlyHighlightMod) {
        AddHelpCommand();
		AddCommand("Highlight",  static_cast<CModCommand::ModCmdFunc>(&OnlyHighlightMod::HandleHighlightCommand),
                   "[highlight]", "Set or display the current highlight");
		AddCommand("LinesToBuffer",  static_cast<CModCommand::ModCmdFunc>(&OnlyHighlightMod::HandleLinesToBufferCommand),
                   "[lines]", "Set or display the number of lines to buffer");
		AddCommand("Channels", static_cast<CModCommand::ModCmdFunc>(&OnlyHighlightMod::HandleChannelsCommand),
                   "[channels]", "Set or display the channels watched");
    }

    virtual ~OnlyHighlightMod() {}

    virtual bool OnLoad(const CString& sArgs, CString& sMessage) {
        CString lines;

        if (!GetUser()) {
            sMessage = "This module must be loaded as a user module";
            return false;
        }

        highlight = GetNV("highlight");
        if (highlight.empty()) {
            highlight = GetUser()->GetNick();
        }
        DEBUG("The highlight is " + highlight);

        lines = GetNV("linesToBuffer");
        if (lines.empty()) {
            linesToBuffer = DEFAULT_LINES_TO_BUFFER;
        }
        else {
            linesToBuffer = lines.ToInt();
        }
        DEBUG("Buffering " + CString(linesToBuffer) + " per channel");

        SetupChannels(GetNV("channels"));

        return true;
    }

    virtual EModRet OnChanMsg(CNick& Nick, CChan& Channel, CString& sMessage) {
        StringIntMap::iterator it;

        /* if a user is connected, don't do anything */
        if (GetUser()->IsUserAttached()) {
            DEBUG("user online, returning CONTINUE");
            return CONTINUE;
        }

        /* check if the module is enabled for this channel */
        it = bufferLineStates.find(Channel.GetName());
        if (it == bufferLineStates.end()) {
            DEBUG("channel [" + Channel.GetName() + "] not watched, "
                  "returning CONTINUE");
            return CONTINUE;
        }

        /* if the message contains a highlight, set the context lines */
        if (sMessage.find(highlight) != CString::npos) {
            DEBUG("message contained highlight: [" + sMessage + "] "
                  "buffering " + CString(linesToBuffer) + " lines");
            it->second = linesToBuffer;
        }

        DEBUG("lines to buffer: " + CString(it->second));

        /* if there are no more lines to buffer, swallow the message */
        if (!it->second) {
            DEBUG("returning HALTCORE");
            return HALTCORE;
        }

        /* consume one line and return the value */
        it->second--;
        DEBUG("returning CONTINUE");
        return CONTINUE;
    }

    void HandleHighlightCommand(const CString& sLine) {
        CString newHighlight = sLine.Token(1);

		if (newHighlight.empty()) {
			PutModule("Current highlight is: " + highlight);
		} else {
            highlight = newHighlight;
            SetNV("highlight", newHighlight);
			PutModule("New highlight is: " + newHighlight);
		}
	}

    void HandleLinesToBufferCommand(const CString& sLine) {
        CString newLinesToBuffer = sLine.Token(1);

		if (newLinesToBuffer.empty()) {
			PutModule("Current number of lines buffered is: " +
                      CString(linesToBuffer));
		} else {
            linesToBuffer = newLinesToBuffer.ToInt();
            SetNV("linesToBuffer", newLinesToBuffer);
			PutModule("New number of lines to buffer is: " + newLinesToBuffer);
		}
	}

    void HandleChannelsCommand(const CString& sLine) {
        CString newChannels = sLine.Token(1, true);

		if (newChannels.empty()) {
            OutputChannels();
		} else {
            SetNV("channels", newChannels);
            SetupChannels(newChannels);
            OutputChannels();
		}
	}

private:

    CString highlight;
    int linesToBuffer;
    typedef map<CString, int> StringIntMap;
    StringIntMap bufferLineStates;

    void OutputChannels(void) {
        CTable table;
        StringIntMap::iterator it;
        unsigned int i = 0;
        CString line;

        if (bufferLineStates.empty()) {
            PutModule("No channels are currently watched");
            return;
        }

        table.AddColumn("channel");
        table.AddColumn("lines left");
        for (it = bufferLineStates.begin();
             it != bufferLineStates.end(); it++) {
            table.AddRow();
            table.SetCell("channel", it->first);
            table.SetCell("lines left", CString(it->second));
        }

        PutModule("Currently watched channels are:");
        while (table.GetLine(i++, line)) {
            PutModule(line);
        }
    }

    void SetupChannels(const CString &channelList) {
        SCString channels;
        SCString::iterator it;

        bufferLineStates.clear();

        channelList.Split(" ", channels, false);
        for (it = channels.begin(); it != channels.end(); it++) {
            DEBUG("Watching channel " + *it);
            bufferLineStates[*it] = 0;
        }
    }

};

MODULEDEFS(OnlyHighlightMod, "Only process messages after a highlight")
