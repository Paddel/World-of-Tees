
#include <engine/textrender.h>
#include <engine/storage.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>

#include "mediaplayer.h"

#define MEDIA_FILE_NAME "Playlists.media"
#define MEDIA_MAX_VOLUME 1000

static char *s_aFileFormats[] = {"3gp", "act", "aiff", "aac", "amr", "ape", "au", "awb", "dct", "dss",
	"dvf", "flac", "gsm", "iklax", "ivs", "m4a", "mmf", "mp3", "mpc", "msv", "ogg", "oga", "opus",
	"ra", "rm", "raw", "sln", "tta", "vox", "wav", "wma", "wv", "webm"};

static char s_PlayString[] = { -30, -106, -70 };

CMediaFile::CMediaFile(char *pFileName)
{
	//Set Filename
	str_copy(m_aFileName, pFileName, sizeof(m_aFileName));

	//Clear Filename for name
	str_copy(m_aName, pFileName, sizeof(m_aName));

	//clear file ending
	const char *pDot = str_find(m_aName, ".");
	if(pDot)
	{
		for(int i = str_length(m_aName)-1; i >= 0; i--)
		{
			char c = m_aName[i];
			m_aName[i] = 0;
			if(c == '.')
				break;
		}
	}

	//clear pathing
	char *pPathPart = (char *)str_find(m_aName, "/");
	if(!pPathPart)
		pPathPart = (char *)str_find(m_aName, "\\");
	while(pPathPart)
	{
		int len = str_length(m_aName)-str_length(pPathPart);
		mem_move(m_aName, m_aName+len+1, str_length(pPathPart));

		pPathPart = (char *)str_find(m_aName, "/");
		if(!pPathPart)
			pPathPart = (char *)str_find(m_aName, "\\");
	}

	m_NotFound = false;
}

CMediaPlaylist::CMediaPlaylist(char *pName)
{
	str_copy(m_aName, pName, sizeof(m_aName));
	m_Default = false;
}

CMediaPlaylist::CMediaPlaylist()
{
	str_copy(m_aName, "Default", sizeof(m_aName));
	m_Default = true;
}

CMediaPlayer::CMediaPlayer()
{
	mem_zero(&m_aCurrentTrack, sizeof(m_aCurrentTrack));
	m_SelectedPlaylist = 0;
	m_SelectedTrack = -1;
	m_PlayingPlaylist = -1;
	m_PlayingTrack = -1;
	m_pPlaylists.add( new CMediaPlaylist());//add default playlist
	m_Action = ACTION_NONE;
	m_RemovingPlaylist = -1;
	m_AddingPlaylistTrack = -1;
	m_TrackPaused = false;
	m_Loop = 1;
}

bool CMediaPlayer::ReadMediaFile()
{
	char aPath[256];
	str_format(aPath, sizeof(aPath), "xclient/%s", MEDIA_FILE_NAME);
	IOHANDLE MediaFile = Storage()->OpenFile(aPath, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!MediaFile)
		return false;

	CLineReader LineReader;
	LineReader.Init(MediaFile);

	// read each line
	char aBuf[512];
	char *pCurrentPlaylist = 0;
	while(char *pLine = LineReader.Get())
	{
		// skip blank/empty lines as well as comments
		if(str_length(pLine) > 0 && pLine[0] != '#' && pLine[0] != '\n' && pLine[0] != '\r'
			&& pLine[0] != '\t' && pLine[0] != '\v' && pLine[0] != ' ')
		{
			if(pLine[0]== '[')
			{
				// new playlist, get the name
				pLine++;

				str_copy(aBuf, pLine, str_length(pLine));
				pCurrentPlaylist = aBuf;
				AddPlaylist(aBuf, false);
			}
			else
			{
				if(pCurrentPlaylist)
				{
					int PlaylistID = GetPlaylistID(pCurrentPlaylist);
					if(PlaylistID != -1)
						AddTrack(PlaylistID, pLine, false);
				}
			}
		}
	}

	return true;
}

bool CMediaPlayer::SaveMediaFile()
{
	char aPath[256];
	str_format(aPath, sizeof(aPath), "xclient/%s", MEDIA_FILE_NAME);
	IOHANDLE MediaFile = Storage()->OpenFile(aPath, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!MediaFile)
		return false;

	for(int i = 0; i < m_pPlaylists.size(); i++)
	{
		CMediaPlaylist *pPlaylist = m_pPlaylists[i];
		char aLine[256];

		str_format(aLine, sizeof(aLine), "[%s]", pPlaylist->m_aName);
		io_write(MediaFile, aLine, str_length(aLine));
		io_write(MediaFile, "\n", str_length("\n"));

		for(int j = 0; j < pPlaylist->m_pFiles.size(); j++)
		{
			str_format(aLine, sizeof(aLine), "%s", pPlaylist->m_pFiles[j]->m_aFileName);
			io_write(MediaFile, aLine, str_length(aLine));
			io_write(MediaFile, "\n", str_length("\n"));
		}
	}

	io_close(MediaFile);
	return true;
}

void CMediaPlayer::OnInit()
{
	//load file
	ReadMediaFile();
}

void CMediaPlayer::OnShutdown()
{
	SaveMediaFile();
}

void CMediaPlayer::OnConsoleInit()
{
	Console()->Register("media_play", "s", CFGFLAG_CLIENT, ConMediaPlay, this, "");
	Console()->Register("media_volume_set", "i", CFGFLAG_CLIENT, ConMediaVolumeSet, this, "");
	Console()->Register("media_pos_get", "", CFGFLAG_CLIENT, ConMediaPosGet, this, "");
	Console()->Register("media_pos_set", "i", CFGFLAG_CLIENT, ConMediaPosSet, this, "");
	Console()->Register("media_playlist_add", "s", CFGFLAG_CLIENT, ConMediaPlaylistAdd, this, "");
	Console()->Register("media_playlist_del", "s", CFGFLAG_CLIENT, ConMediaPlaylistDel, this, "");
	Console()->Register("media_track_add", "ss", CFGFLAG_CLIENT, ConMediaTrackAdd, this, "");
	Console()->Register("media_track_del", "ss", CFGFLAG_CLIENT, ConMediaTrackDel, this, "");
}

void CMediaPlayer::OnTick()
{
	if(m_PlayingPlaylist != -1 && m_PlayingPlaylist >= 0 && m_PlayingPlaylist < m_pPlaylists.size())
	{
		CMediaPlaylist *pPlaylist = m_pPlaylists[m_PlayingPlaylist];
		if(pPlaylist->m_pFiles.size() > 0 && m_PlayingTrack >= 0 && m_PlayingTrack < pPlaylist->m_pFiles.size())
		{
			CMediaFile *pCurrentTrack = pPlaylist->m_pFiles[m_PlayingTrack];
			if(str_comp(pCurrentTrack->m_aFileName, m_aCurrentTrack) != 0)
			{
				if(Play(pCurrentTrack->m_aFileName) == false)
				{
					if(NextTrack() == false)
						m_PlayingTrack = -1;
				}
			}
			else
			{
				if(GetPos() >= GetLength())//song over
				{
					if(m_Loop == 2)//loop the song
					{
						SetPos(0);
					}
					else
					{
						if(NextTrack() == false)
							m_PlayingTrack = -1;
					}
				}
			}
		}
	}
}

void CMenus::MediaRender(CUIRect MainView)
{
	if(MediaPlayer()->GetAction() == CMediaPlayer::ACTION_NONE)
	{
		CUIRect Playlist, Filelist, ControlPanel;
		MainView.VSplitLeft(240.0f, &Playlist, &Filelist);
		Filelist.VSplitLeft(8.0f, NULL, &Filelist);

		MediaRenderPlayList(Playlist);
		Filelist.HSplitBottom(100.0f, &Filelist, &ControlPanel);
		MediaRenderTracks(Filelist);
		MediaRenderControlPanel(ControlPanel);
	}
	else if(MediaPlayer()->GetAction() == CMediaPlayer::ACTION_PLAYLIST_REMOVE)
	{
		PopupPlaylistRemove(MainView);
	}
	else if(MediaPlayer()->GetAction() == CMediaPlayer::ACTION_PLAYLIST_ADD)
	{
		PopupPlaylistAdd(MainView);
	}
	else if(MediaPlayer()->GetAction() == CMediaPlayer::ACTION_TRACK_ADD)
	{
		PopupTrackAdd(MainView);
	}
}

void CMenus::MediaRenderPlayList(CUIRect MainView)
{
	CUIRect Button, Picker;
	array<CMediaPlaylist *> pPlayLists = MediaPlayer()->GetPlayLists();
	static float s_ScrollValue = 0.0f;
	int OldSelected = MediaPlayer()->GetSelectedPlaylist();

	MainView.HSplitBottom(32.0f, &Picker, &Button);

	UiDoListboxStart(&s_ScrollValue, &Picker, 32.0f, Localize("Playlists"), "", pPlayLists.size(), 1, OldSelected, s_ScrollValue);

	for(int i = 0; i < pPlayLists.size(); ++i)
	{
		CMediaPlaylist *pPlayerlist = pPlayLists[i];
		CListboxItem Item = UiDoListboxNextItem(pPlayerlist, MediaPlayer()->GetSelectedPlaylist() == i);
		if(Item.m_Visible)
		{
			CUIRect Label;
			Item.m_Rect.Margin(5.0f, &Item.m_Rect);
			Item.m_Rect.HSplitTop(0, &Item.m_Rect, &Label);

			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

			UI()->DoLabel(&Label, pPlayerlist->m_aName, 18.0f, -1);

			if(MediaPlayer()->GetPlaylingPlaylist() == i)
				UI()->DoLabel(&Label, "<", 9.0f, 1);
		}
	}

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	bool Activated = 0;
	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, &Activated);
	if(MediaPlayer()->GetSelectedPlaylist() != NewSelected)
	{
		MediaPlayer()->SetSelectedPlaylist(NewSelected);
	}

	if(Activated)
		MediaPlayer()->PlayPlaylist(NewSelected);

	CUIRect ButtonTop, ButtonBottom, ButtonTopLeft, ButttonTopRight;
	Button.HSplitMid(&ButtonTop, &ButtonBottom);
	ButtonTop.VSplitMid(&ButtonTopLeft, &ButttonTopRight);
	bool SelectionValid = MediaPlayer()->GetSelectedPlaylist() >= 0 & MediaPlayer()->GetSelectedPlaylist() < pPlayLists.size();
	vec4 Color = vec4(1,1,1,0.5f);
	if(SelectionValid == false)
		Color = vec4(0.1f,0.1f,0.1f,0.8f);

	static int s_ButtonRemove = -1;
	if(DoButton_Menu(&s_ButtonRemove, "Remove", 1, &ButtonTopLeft, NULL, (SelectionValid&&pPlayLists[MediaPlayer()->GetSelectedPlaylist()]->m_Default==false)?vec4(1,1,1,0.5f):vec4(0.1f,0.1f,0.1f,0.8f), CUI::CORNER_T&CUI::CORNER_L) && SelectionValid)
	{
		MediaPlayer()->PreRemovePlaylist(MediaPlayer()->GetSelectedPlaylist());
	}

	static int s_ButtonAdd = -1;
	if(DoButton_Menu(&s_ButtonAdd, "Add", 1, &ButttonTopRight, NULL, vec4(1,1,1,0.5f), CUI::CORNER_T&CUI::CORNER_R))
	{
		MediaPlayer()->PreAddPlaylist();
	}

	static int s_ButtonPlay = -1;
	if(DoButton_Menu(&s_ButtonPlay, "Play", 1, &ButtonBottom, NULL, Color, CUI::CORNER_B) && SelectionValid)
	{
		MediaPlayer()->PlayPlaylist(MediaPlayer()->GetSelectedPlaylist());
	}

}

void CMenus::MediaRenderTracks(CUIRect MainView)
{
	CUIRect Button, Picker;
	array<CMediaPlaylist *> pPlayLists = MediaPlayer()->GetPlayLists();
	int SelectedPlaylist = MediaPlayer()->GetSelectedPlaylist();
	CMediaPlaylist *pSelectedPlaylist = pPlayLists[SelectedPlaylist];
	static float s_ScrollValue = 0.0f;
	int OldSelected = MediaPlayer()->GetSelectedTrack();

	if(SelectedPlaylist < 0 || SelectedPlaylist >= pPlayLists.size())
		return;

	MainView.HSplitBottom(24.0f, &Picker, &Button);

	UiDoListboxStart(&s_ScrollValue, &Picker, 20.0f, Localize("Tracks"), "", pSelectedPlaylist->m_pFiles.size(), 1, OldSelected, s_ScrollValue);

	for(int i = 0; i < pSelectedPlaylist->m_pFiles.size(); ++i)
	{
		CMediaFile *pMediaFile = pSelectedPlaylist->m_pFiles[i];
		CListboxItem Item = UiDoListboxNextItem(pMediaFile, MediaPlayer()->GetSelectedTrack() == i);
		if(Item.m_Visible)
		{
			CUIRect Label;
			Item.m_Rect.Margin(5.0f, &Item.m_Rect);
			Item.m_Rect.HSplitTop(0, &Item.m_Rect, &Label);

			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

			UI()->DoLabel(&Label, pMediaFile->m_aName, 9.0f, -1);

			if(pMediaFile->m_NotFound)
				UI()->DoLabel(&Label, "!", 9.0f, 1);
			else if(MediaPlayer()->GetPlaylingTrack() == i && MediaPlayer()->GetPlaylingPlaylist() == SelectedPlaylist)
				UI()->DoLabel(&Label, "<", 9.0f, 1);
		}
	}

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	bool Activated = 0;
	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, &Activated);
	if(OldSelected != NewSelected)
	{
		MediaPlayer()->SetSelectedTrack(NewSelected);
		if(NewSelected >= 0 && NewSelected < pSelectedPlaylist->m_pFiles.size())
			MediaPlayer()->CheckTrackFound(pSelectedPlaylist->m_pFiles[NewSelected]);
	}

	if(Activated)
		MediaPlayer()->PlayTrack(MediaPlayer()->GetSelectedPlaylist(), NewSelected);

	CUIRect ButtonLeft, ButtonMid, ButtonRight;
	float Width = Button.w;
	bool SelectionValid = MediaPlayer()->GetSelectedTrack() >= 0 & MediaPlayer()->GetSelectedTrack() < pSelectedPlaylist->m_pFiles.size();
	Button.VSplitLeft(Width*0.3f, &ButtonLeft, &Button);
	Button.VSplitLeft(Width*0.3f, &ButtonMid, &ButtonRight);

	vec4 Color = vec4(1,1,1,0.5f);
	if(SelectionValid == false)
		Color = vec4(0.1f,0.1f,0.1f,0.8f);

	static int s_ButtonAdd = -1;
	if(DoButton_Menu(&s_ButtonAdd, "Add", 1, &ButtonLeft, NULL, vec4(1,1,1,0.5f), CUI::CORNER_L))
	{
		if(SelectedPlaylist >= 0 && SelectedPlaylist < pPlayLists.size())
			MediaPlayer()->PreAddTrack(SelectedPlaylist);
	}

	static int s_ButtonPlay = -1;
	if(DoButton_Menu(&s_ButtonPlay, "Play", 1, &ButtonMid, NULL, Color, 0) && SelectionValid)
	{
		MediaPlayer()->PlayTrack(SelectedPlaylist, MediaPlayer()->GetSelectedTrack());
	}

	static int s_ButtonRemove = -1;
	if(DoButton_Menu(&s_ButtonRemove, "Remove", 1, &ButtonRight, NULL, Color, CUI::CORNER_R) && SelectionValid)
	{
		MediaPlayer()->RemoveTrack(SelectedPlaylist, MediaPlayer()->GetSelectedTrack());
	}
}

void CMenus::MediaRenderControlPanel(CUIRect MainView)
{
	CUIRect Line, PlayButtom;
	float Height = MainView.h;
	MainView.HSplitTop(Height*0.3f, &Line, &MainView);
	{//line 1
		Line.VSplitLeft(Line.h, &PlayButtom, &Line);

		vec4 Color = vec4(1,1,1,0.5f);
		if(MediaPlayer()->IsPlaying() == false)
			Color = vec4(0.1f,0.1f,0.1f,0.8f);

		static int s_ButtonPause = -1;
		if(DoButton_Menu(&s_ButtonPause, MediaPlayer()->TrackPaused()? s_PlayString : "| |", 1, &PlayButtom, NULL, Color))
		{
			if(MediaPlayer()->IsPlaying())
				MediaPlayer()->TogglePause();
		}

		Line.VSplitLeft(10.0f, NULL, &Line);
		static int s_Volume = g_Config.m_XMediaVolume;
		s_Volume = (int)(DoScrollbarH(&s_Volume, &Line, s_Volume/(float)MEDIA_MAX_VOLUME)*(float)MEDIA_MAX_VOLUME);
		if(s_Volume != g_Config.m_XMediaVolume)
			MediaPlayer()->SetVolume(s_Volume);
	}

	MainView.HSplitTop(Height*0.3f, &Line, &MainView);
	{//line 2
		CUIRect Left, Mid, Right;
		Line.VSplitLeft(Line.h+8, &Left, &Line);
		Line.VSplitRight(Line.h+8, &Mid, &Right);
		static void *pLastActiveItem = NULL;
		static int s_ShownPos = MediaPlayer()->GetPos();
		float Length = MediaPlayer()->IsPlaying()? (float)MediaPlayer()->GetLength()-10 : 1.0f;
		static int s_PositionScrollbar = -1;
		s_ShownPos = (int)(DoScrollbarH(&s_PositionScrollbar, &Mid, s_ShownPos/Length)*Length);
		if(pLastActiveItem == &s_PositionScrollbar && UI()->ActiveItem() != &s_PositionScrollbar)
			MediaPlayer()->SetPos(s_ShownPos);
		else if(UI()->ActiveItem() == &s_PositionScrollbar)
			MediaPlayer()->Pause();
		else
			s_ShownPos = MediaPlayer()->GetPos();

		pLastActiveItem = (void *)UI()->ActiveItem();

		static int s_ButtonPrefTrack = -1;
		if(DoButton_Menu(&s_ButtonPrefTrack, "<<", 1, &Left, NULL))
			MediaPlayer()->PrevTrack();

		static int s_ButtonNextTrack = -1;
		if(DoButton_Menu(&s_ButtonNextTrack, ">>", 1, &Right, NULL))
			MediaPlayer()->NextTrack();

	}
	MainView.HSplitTop(Height*0.3f, &Line, NULL);
	{//line 3
		int Width = Line.w;
		static int s_ButtonRandom = 0;
		static int s_ButtonLoopAll = 0;
		static int s_ButtonLoopOne = 0;
		int *pLoop = MediaPlayer()->Loop();
		if(DoButton_CheckBox(&s_ButtonRandom, Localize("Random"), *pLoop == 4, &Line))
		{
			if(*pLoop == 4)
				*pLoop = 0;
			else
				*pLoop = 4;
		}

		Line.VSplitLeft(Width*0.3f, NULL, &Line);
		if(DoButton_CheckBox(&s_ButtonLoopAll, Localize("Loop all"), *pLoop == 1, &Line))
		{
			if(*pLoop == 1)
				*pLoop = 0;
			else
				*pLoop = 1;
		}

		Line.VSplitLeft(Width*0.3f, NULL, &Line);
		if(DoButton_CheckBox(&s_ButtonLoopOne, Localize("Loop one"), *pLoop == 2, &Line))
		{
			if(*pLoop == 2)
				*pLoop = 0;
			else
				*pLoop = 2;
		}
	}
}

void CMenus::PopupPlaylistRemove(CUIRect MainView)
{
	char aBuf[256];
	CUIRect Yes, No, Part, Box = MainView;
	int ListID = MediaPlayer()->GetRemovingPlaylist();
	array<CMediaPlaylist *> pPlayLists = MediaPlayer()->GetPlayLists();

	if(ListID < 0 || ListID >= pPlayLists.size())//not a available playlist
	{
		MediaPlayer()->SetAction(CMediaPlayer::ACTION_NONE);
		return;
	}
		
	CMediaPlaylist *pPlaylist = pPlayLists[ListID];

	str_format(aBuf, sizeof(aBuf), "Are you sure you want to remove the playlist '%s'", pPlaylist->m_aName);

	Box.HSplitTop(20.f/UI()->Scale(), &Part, &Box);
	Box.HSplitTop(24.f/UI()->Scale(), &Part, &Box);
	UI()->DoLabelScaled(&Part, aBuf, 24.f, 0);
	Box.HSplitTop(20.f/UI()->Scale(), &Part, &Box);
	Box.HSplitTop(24.f/UI()->Scale(), &Part, &Box);
	Part.VMargin(20.f/UI()->Scale(), &Part);

	Box.HSplitBottom(20.f, &Box, &Part);
	Box.HSplitBottom(24.f, &Box, &Part);

	// additional info
	Box.HSplitTop(10.0f, 0, &Box);
	Box.VMargin(20.f/UI()->Scale(), &Box);

	// buttons
	Part.VMargin(80.0f, &Part);
	Part.VSplitMid(&No, &Yes);
	Yes.VMargin(20.0f, &Yes);
	No.VMargin(20.0f, &No);

	static int s_ButtonAbort = 0;
	if(DoButton_Menu(&s_ButtonAbort, Localize("No"), 0, &No) || m_EscapePressed)
		MediaPlayer()->SetAction(CMediaPlayer::ACTION_NONE);

	static int s_ButtonTryAgain = 0;
	if(DoButton_Menu(&s_ButtonTryAgain, Localize("Yes"), 0, &Yes) || m_EnterPressed)
	{
		MediaPlayer()->RemovePlaylist(ListID);
		MediaPlayer()->SetAction(CMediaPlayer::ACTION_NONE);
	}
}

void CMenus::PopupPlaylistAdd(CUIRect MainView)
{
	CUIRect Label, TextBox, Box = MainView, Part, Yes, No;
	static char s_aBuf[64];

	Box.HSplitBottom(20.f, &Box, &Part);
	Box.HSplitBottom(24.f, &Box, &Part);

	// buttons
	Part.VMargin(80.0f, &Part);
	Part.VSplitMid(&No, &Yes);
	Yes.VMargin(20.0f, &Yes);
	No.VMargin(20.0f, &No);

	static int s_ButtonAbort = 0;
	if(DoButton_Menu(&s_ButtonAbort, Localize("Abort"), 0, &No) || m_EscapePressed)
		MediaPlayer()->SetAction(CMediaPlayer::ACTION_NONE);

	static int s_ButtonTryAgain = 0;
	if(DoButton_Menu(&s_ButtonTryAgain, Localize("Enter"), 0, &Yes) || m_EnterPressed)
	{
		MediaPlayer()->AddPlaylist(s_aBuf);
		MediaPlayer()->SetAction(CMediaPlayer::ACTION_NONE);
	}

	Box.HSplitBottom(40.f, &Box, &Part);
	Box.HSplitBottom(24.f, &Box, &Part);

	Part.VSplitLeft(60.0f, 0, &Label);
	Label.VSplitLeft(100.0f, 0, &TextBox);
	TextBox.VSplitLeft(20.0f, 0, &TextBox);
	TextBox.VSplitRight(60.0f, &TextBox, 0);
	UI()->DoLabel(&Label, Localize("Name"), 18.0f, -1);
	static float Offset = 0.0f;
	DoEditBox(&s_aBuf, &TextBox, s_aBuf, sizeof(s_aBuf), 12.0f, &Offset);
}

static int MediaPlayerListdirCallback(const char *pName, int IsDir, int StorageType, void *pUser)
{
	sorted_array<CMediaAddEntry> *pList = (sorted_array<CMediaAddEntry>*)pUser;
	if(IsDir == false)
	{
		int NumFormats = sizeof(s_aFileFormats)/sizeof(s_aFileFormats[0]);
		bool FormatFound = false;
		for(int i = 0; i < NumFormats; i++)
		{
			if(str_comp(pName+(str_length(pName)-str_length(s_aFileFormats[i])), s_aFileFormats[i]) == 0)
			{
				FormatFound = true;
				break;
			}
		}
		if(FormatFound == false)
			return 0;
	}
	else if(str_comp(pName, ".") == 0)
		return 0;
	CMediaAddEntry NewEntry = CMediaAddEntry((char *)pName, (bool) IsDir);
	pList->add(NewEntry);
	return 0;
}

void CMenus::PopupTrackAdd(CUIRect MainView)
{
	CUIRect Selector, Button;
	static char s_aCurrenPath[256];
	static float s_ScrollValue = 0.0f;
	static int s_Selected = -1;

	if(!s_aCurrenPath[0])
	{
		MediaPlayer()->m_AddEntries.clear();
		str_copy(s_aCurrenPath, Storage()->GetCurrentPath(), sizeof(s_aCurrenPath));
		fs_listdir(s_aCurrenPath, MediaPlayerListdirCallback, 0, &MediaPlayer()->m_AddEntries);
	}

	MainView.HSplitBottom(24.0f, &Selector, &Button);

	UiDoListboxStart(&s_ScrollValue, &Selector, 20.0f, Localize("Add Track"), "", MediaPlayer()->m_AddEntries.size(), 1, s_Selected, s_ScrollValue);	

	for(int i = 0; i < MediaPlayer()->m_AddEntries.size(); i++)
	{
		CMediaAddEntry *pMediaFile = &MediaPlayer()->m_AddEntries[i];
		CListboxItem Item = UiDoListboxNextItem(pMediaFile, s_Selected == i);
		if(Item.m_Visible)
		{
			CUIRect Label;
			Item.m_Rect.Margin(5.0f, &Item.m_Rect);
			Item.m_Rect.HSplitTop(0, &Item.m_Rect, &Label);

			if(pMediaFile->m_Folder)
				TextRender()->TextColor(0.5f, 0.5f, 0.5f, 1.0f);
			else
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

			UI()->DoLabel(&Label, pMediaFile->m_aName, 9.0f, -1);
		}
	}

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	bool Activated = 0;
	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, &Activated);
	if(s_Selected != NewSelected)
	{
		s_Selected = NewSelected;
	}

	if(Activated)
	{
		CMediaAddEntry *pMediaFile = &MediaPlayer()->m_AddEntries[s_Selected];
		if(pMediaFile->m_Folder)
		{
			if(str_comp(pMediaFile->m_aName, "..") == 0)
			{
				for(int i = str_length(s_aCurrenPath)-1; i >= 0; i--)
				{
					char c = s_aCurrenPath[i];
					s_aCurrenPath[i] = 0;
					if(c == '\\' || c == '/')
						break;
				}
			}
			else
				str_format(s_aCurrenPath, sizeof(s_aCurrenPath), "%s\\%s", s_aCurrenPath[0]?s_aCurrenPath:"", pMediaFile->m_aName);

			MediaPlayer()->m_AddEntries.clear();
			fs_listdir(s_aCurrenPath, MediaPlayerListdirCallback, 0, &MediaPlayer()->m_AddEntries);
		}
		else
		{
			char aFileName[256];
			str_format(aFileName, sizeof(aFileName), "%s\\%s", s_aCurrenPath[0]?s_aCurrenPath:"", pMediaFile->m_aName);
			MediaPlayer()->AddTrack(MediaPlayer()->GetAddingPlaylistTrack(), aFileName);
			MediaPlayer()->SetAction(CMediaPlayer::ACTION_NONE);
		}
	}

	CUIRect ButtonLeft, ButtonMid, ButtonRight;
	float Width = Button.w;
	Button.VSplitRight(Width*0.3f, &Button, &ButtonRight);
	Button.VSplitRight(Width*0.3f, &ButtonLeft, &ButtonMid);

	CMediaAddEntry *pMediaFile = &MediaPlayer()->m_AddEntries[s_Selected];
	bool ValidSelection = false;
	if(s_Selected >= 0 && s_Selected < MediaPlayer()->m_AddEntries.size())
		ValidSelection = true;

	vec4 Color = vec4(1,1,1,0.5f);
	if(ValidSelection == false)
		Color = vec4(0.1f,0.1f,0.1f,0.8f);

	if(ValidSelection == false || pMediaFile->m_Folder == false)
	{
		static int s_ButtonAdd = -1;
		if(DoButton_Menu(&s_ButtonAdd, "Add", 1, &ButtonLeft, NULL, Color, CUI::CORNER_L) && ValidSelection)
		{
			CMediaAddEntry *pMediaFile = &MediaPlayer()->m_AddEntries[s_Selected];
			char aFileName[256];
			str_format(aFileName, sizeof(aFileName), "%s\\%s", s_aCurrenPath[0]?s_aCurrenPath:"", pMediaFile->m_aName);
			MediaPlayer()->AddTrack(MediaPlayer()->GetAddingPlaylistTrack(), aFileName);
			MediaPlayer()->SetAction(CMediaPlayer::ACTION_NONE);
		}

		static int s_ButtonAddNoClose = -1;
		if(DoButton_Menu(&s_ButtonAddNoClose, "Add (no close)", 1, &ButtonMid, NULL, Color, 0) && ValidSelection)
		{
			CMediaAddEntry *pMediaFile = &MediaPlayer()->m_AddEntries[s_Selected];
			char aFileName[256];
			str_format(aFileName, sizeof(aFileName), "%s\\%s", s_aCurrenPath[0]?s_aCurrenPath:"", pMediaFile->m_aName);
			MediaPlayer()->AddTrack(MediaPlayer()->GetAddingPlaylistTrack(), aFileName);
		}
	}
	else
	{
		static int s_ButtonOpen = -1;
		if(DoButton_Menu(&s_ButtonOpen, "Open", 1, &Button, NULL, vec4(1,1,1,0.5f), CUI::CORNER_L))
		{
			if(str_comp(pMediaFile->m_aName, "..") == 0)
			{
				for(int i = str_length(s_aCurrenPath)-1; i >= 0; i--)
				{
					char c = s_aCurrenPath[i];
					s_aCurrenPath[i] = 0;
					if(c == '\\' || c == '/')
						break;
				}
			}
			else
				str_format(s_aCurrenPath, sizeof(s_aCurrenPath), "%s\\%s", s_aCurrenPath[0]?s_aCurrenPath:"", pMediaFile->m_aName);

			MediaPlayer()->m_AddEntries.clear();
			fs_listdir(s_aCurrenPath, MediaPlayerListdirCallback, 0, &MediaPlayer()->m_AddEntries);
		}
	}

	static int s_ButtonClose = -1;
	if(DoButton_Menu(&s_ButtonClose, "Close", 1, &ButtonRight, NULL, vec4(1,1,1,0.5f), CUI::CORNER_R))
	{
		MediaPlayer()->SetAction(CMediaPlayer::ACTION_NONE);
	}
}

bool CMediaPlayer::Play(char *pFileName)
{
	if(IsPlaying())
	{
		if(str_comp(m_aCurrentTrack, pFileName) == 0)
			return false;
		else
			Stop();

	}

	if(media_play(pFileName) == 0)
		return false;

	str_copy(m_aCurrentTrack, pFileName, sizeof(m_aCurrentTrack));
	MediaPlayer()->SetVolume(g_Config.m_XMediaVolume);
	return true;
}

void CMediaPlayer::Pause()
{
	if(IsPlaying() == false || m_TrackPaused)
		return;

	media_pause(m_aCurrentTrack);
}

void CMediaPlayer::Resume()
{
	if(IsPlaying() == false || m_TrackPaused == false)
		return;

	media_resume(m_aCurrentTrack);
}

void CMediaPlayer::Stop()
{
	if(IsPlaying() == false)
		return;

	media_close(m_aCurrentTrack);
	mem_zero(&m_aCurrentTrack, sizeof(m_aCurrentTrack));
}

void CMediaPlayer::SetVolume(int NewVolume)
{
	media_setvolume(m_aCurrentTrack, NewVolume);
	g_Config.m_XMediaVolume = NewVolume;
}

void CMediaPlayer::SetPos(int Position)
{
	if(!IsPlaying())
		return;

	Position = clamp(Position, 0, GetLength());

	media_pos_set(m_aCurrentTrack, Position);
	media_resume(m_aCurrentTrack);
}

int CMediaPlayer::GetPos()
{
	if(!IsPlaying())
		return 0;

	return media_pos_get(m_aCurrentTrack);
}

int CMediaPlayer::GetLength()
{
	if(!IsPlaying())
		return 0;

	return media_length(m_aCurrentTrack);
}

bool CMediaPlayer::IsPlaying()
{
	if(m_aCurrentTrack[0])
		return true;
	else
		return false;
}

int CMediaPlayer::GetPlaylistID(char *pName)
{
	for(int i = 0; i < m_pPlaylists.size(); i++)
	{
		if(str_comp(m_pPlaylists[i]->m_aName, pName) == 0)
			return i;
	}
	return -1;
}

int CMediaPlayer::GetTrackID(int PlaylistID, char *pName)
{
	if(PlaylistID < 0 || PlaylistID >= m_pPlaylists.size())
		return -1;//not a available playlist

	CMediaPlaylist *pPlaylist = m_pPlaylists[PlaylistID];

	for(int i = 0; i < pPlaylist->m_pFiles.size(); i++)
	{
		if(str_comp(pPlaylist->m_pFiles[i]->m_aName, pName) == 0)
			return i;
	}
	return -1;
}

void CMediaPlayer::PreAddPlaylist()
{
	m_Action = ACTION_PLAYLIST_ADD;
}

bool CMediaPlayer::AddPlaylist(char *pName, bool Save)
{
	for(int i = 0; i < m_pPlaylists.size(); i++)
	{
		if(str_comp(m_pPlaylists[i]->m_aName, pName) == 0)//already in list
			return false;
	}
	m_pPlaylists.add( new CMediaPlaylist(pName));

	if(Save)
		SaveMediaFile();

	return true;
}

bool CMediaPlayer::PreRemovePlaylist(int PlaylistID)
{
	if(PlaylistID < 0 || PlaylistID >= m_pPlaylists.size())
		return false;//not a available playlist

	if(m_pPlaylists[PlaylistID]->m_Default)//cannnot remove default playlists
		return false;

	m_Action = ACTION_PLAYLIST_REMOVE;
	m_RemovingPlaylist = PlaylistID;
	return true;
}

bool CMediaPlayer::RemovePlaylist(int PlaylistID)
{
	if(PlaylistID < 0 || PlaylistID >= m_pPlaylists.size())
		return false;//not a available playlist

	if(m_pPlaylists[PlaylistID]->m_Default)//cannnot remove default playlists
		return false;

	if(m_PlayingPlaylist == PlaylistID)
	{
		m_PlayingPlaylist = -1;
		m_PlayingTrack = -1;
		Stop();
	}

	m_pPlaylists.remove_index(PlaylistID);
	SaveMediaFile();
	return true;
}


void CMediaPlayer::PlayPlaylist(int Index)
{
	if(Index < 0 || Index >= m_pPlaylists.size() || Index == m_PlayingPlaylist)
		return;//not a available playlist

	if(Index != m_PlayingPlaylist)
		Stop();

	m_PlayingPlaylist = Index;
	m_PlayingTrack = 0;
	m_TrackPaused = false;
}

void CMediaPlayer::PlayTrack(int Playlist, int Index)
{
	if(Playlist < 0 || Playlist >= m_pPlaylists.size())
		return;

	CMediaPlaylist *pPlaylist = m_pPlaylists[Playlist];

	if(Index < 0 || Index >= pPlaylist->m_pFiles.size())
		return;//not a available playlist

	CMediaFile *pTrack = pPlaylist->m_pFiles[Index];
	if(pTrack->m_NotFound)
		return;

	if(Playlist != m_PlayingPlaylist)
	{
		m_PlayingPlaylist = Playlist;
		Stop();
	}
	else if(Index == m_PlayingTrack)
		SetPos(0);

	m_PlayingTrack = Index;
	m_TrackPaused = false;
}

void CMediaPlayer::PreAddTrack(int Playlist)
{
	if(Playlist < 0 || Playlist >= m_pPlaylists.size())
		return;

	m_AddingPlaylistTrack = Playlist;
	m_Action = ACTION_TRACK_ADD;
}

bool CMediaPlayer::AddTrack(int PlaylistID, char *pFileName, bool Save)
{
	if(PlaylistID < 0 || PlaylistID >= m_pPlaylists.size())
		return false;//not a available playlist

	CMediaPlaylist *pPlaylist = m_pPlaylists[PlaylistID];

	for(int i = 0; i < pPlaylist->m_pFiles.size(); i++)
	{
		if(str_comp(pPlaylist->m_pFiles[i]->m_aFileName, pFileName) == 0)
			return false;//already in playlist
	}

	CMediaFile *pNewFile = new CMediaFile(pFileName);
	if(!pNewFile->m_aName[0])//smth went wrong with namecreation probably wrong path
		return false;

	CheckTrackFound(pNewFile);
	pPlaylist->m_pFiles.add(pNewFile);

	if(Save)
		SaveMediaFile();

	return true;
}

bool CMediaPlayer::RemoveTrack(int PlaylistID, int Index)
{
	if(PlaylistID < 0 || PlaylistID >= m_pPlaylists.size())
		return false;//not a available playlist

	CMediaPlaylist *pPlaylist = m_pPlaylists[PlaylistID];
	if(Index < 0 || Index >= pPlaylist->m_pFiles.size())
		return false;

	if(PlaylistID == m_PlayingPlaylist && m_PlayingTrack == Index)
	{
		m_PlayingTrack = -1;
		Stop();
	}

	pPlaylist->m_pFiles.remove_index(Index);
	SaveMediaFile();
	return true;
}

bool CMediaPlayer::NextTrack()
{
	if(m_PlayingPlaylist != -1 && m_PlayingPlaylist >= 0 && m_PlayingPlaylist < m_pPlaylists.size())
	{
		CMediaPlaylist *pPlaylist = m_pPlaylists[m_PlayingPlaylist];

		//check if one track is available
		bool FoundAvailableTrack = false;
		for(int i = 0; i < pPlaylist->m_pFiles.size(); i++)
		{
			CMediaFile *pTrack = pPlaylist->m_pFiles[i];
			if(pTrack->m_NotFound == false)
			{
				FoundAvailableTrack = true;
				break;
			}
		}

		if(FoundAvailableTrack == false)
			return false;

		if(pPlaylist->m_pFiles.size() > 0 && m_PlayingTrack >= 0 && m_PlayingTrack < pPlaylist->m_pFiles.size())
		{
			if(m_Loop != 4)
			{
				m_PlayingTrack++;
				if(m_PlayingTrack >= pPlaylist->m_pFiles.size())
				{
					if(m_Loop == 1)
						m_PlayingTrack = 0;
					else
						m_PlayingTrack = -1;
				}
			}
			else
				m_PlayingTrack = rand()%pPlaylist->m_pFiles.size();

			CMediaFile *pCurrentTrack = pPlaylist->m_pFiles[m_PlayingTrack];
			CheckTrackFound(pCurrentTrack);
			if(pCurrentTrack->m_NotFound)
				NextTrack();

			return true;
		}
	}

	return false;
}

bool CMediaPlayer::PrevTrack()
{
	if(m_PlayingPlaylist != -1 && m_PlayingPlaylist >= 0 && m_PlayingPlaylist < m_pPlaylists.size())
	{
		CMediaPlaylist *pPlaylist = m_pPlaylists[m_PlayingPlaylist];

		//check if one track is available
		bool FoundAvailableTrack = false;
		for(int i = 0; i < pPlaylist->m_pFiles.size(); i++)
		{
			CMediaFile *pTrack = pPlaylist->m_pFiles[i];
			if(pTrack->m_NotFound == false)
			{
				FoundAvailableTrack = true;
				break;
			}
		}

		if(FoundAvailableTrack == false)
			return false;

		if(pPlaylist->m_pFiles.size() > 0 && m_PlayingTrack >= 0 && m_PlayingTrack < pPlaylist->m_pFiles.size())
		{
			if(m_Loop != 4)
			{
				m_PlayingTrack--;
				if(m_PlayingTrack < 0)
				{
					if(m_Loop == 1)
						m_PlayingTrack = pPlaylist->m_pFiles.size()-1;
					else
						m_PlayingTrack = -1;
				}
			}
			else
				m_PlayingTrack = rand()%pPlaylist->m_pFiles.size();

			CMediaFile *pCurrentTrack = pPlaylist->m_pFiles[m_PlayingTrack];
			CheckTrackFound(pCurrentTrack);
			if(pCurrentTrack->m_NotFound)
				PrevTrack();

			return true;
		}
	}

	return false;
}

void CMediaPlayer::CheckTrackFound(CMediaFile *pTrack)
{
	if(media_load(pTrack->m_aFileName) == 1)
	{
		if(PlayingTrack() != pTrack)
			media_close(pTrack->m_aFileName);
	}
	else
		pTrack->m_NotFound = true;
}

CMediaFile *CMediaPlayer::PlayingTrack()
{
	if(m_PlayingPlaylist != -1 && m_PlayingPlaylist >= 0 && m_PlayingPlaylist < m_pPlaylists.size())
	{
		CMediaPlaylist *pPlaylist = m_pPlaylists[m_PlayingPlaylist];
		if(pPlaylist->m_pFiles.size() > 0 && m_PlayingTrack >= 0 && m_PlayingTrack < pPlaylist->m_pFiles.size())
			return pPlaylist->m_pFiles[m_PlayingTrack];
	}
	return NULL;
}

void CMediaPlayer::TogglePause()
{
	if(IsPlaying() == false)
		return;

	if(m_TrackPaused)
		Resume();
	else
		Pause();

	m_TrackPaused = !m_TrackPaused;
}

void CMediaPlayer::SetSelectedPlaylist(int Index)
 {
	 m_SelectedPlaylist = Index;
	 m_SelectedTrack = -1;
}

void CMediaPlayer::ConMediaPlay(IConsole::IResult *pResult, void *pUserData)
{
	CMediaPlayer *pThis = (CMediaPlayer*)pUserData;
	const char *pFileName = pResult->GetString(0);

	pThis->Play((char *)pFileName);
}

void CMediaPlayer::ConMediaVolumeSet(IConsole::IResult *pResult, void *pUserData)
{
	CMediaPlayer *pThis = (CMediaPlayer*)pUserData;
	int Volume = pResult->GetInteger(0);

	pThis->SetVolume(Volume);
}

void CMediaPlayer::ConMediaPosGet(IConsole::IResult *pResult, void *pUserData)
{
	CMediaPlayer *pThis = (CMediaPlayer*)pUserData;
	char aBuf[256];

	str_format(aBuf, sizeof(aBuf), "Song at %i/%i", pThis->GetPos());
	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MediaPlayer", aBuf);
}

void CMediaPlayer::ConMediaPosSet(IConsole::IResult *pResult, void *pUserData)
{
	CMediaPlayer *pThis = (CMediaPlayer*)pUserData;
	int Pos = pResult->GetInteger(0);

	pThis->SetPos(Pos);
}

void CMediaPlayer::ConMediaPlaylistAdd(IConsole::IResult *pResult, void *pUserData)
{
	CMediaPlayer *pThis = (CMediaPlayer*)pUserData;
	const char *pName = pResult->GetString(0);
	char aBuf[256];

	if(pThis->AddPlaylist((char *)pName))
	{
		str_format(aBuf, sizeof(aBuf), "Playlist %s has been added successfully.", pName);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MediaPlayer", aBuf);
	}
	else
	{
		str_format(aBuf, sizeof(aBuf), "Playlist %s could not be added.", pName);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MediaPlayer", aBuf);
	}

}

void CMediaPlayer::ConMediaPlaylistDel(IConsole::IResult *pResult, void *pUserData)
{
	CMediaPlayer *pThis = (CMediaPlayer*)pUserData;
	const char *pName = pResult->GetString(0);
	int PlaylistID = pThis->GetPlaylistID((char *)pName);
	char aBuf[256];

	if(PlaylistID == -1)
	{
		str_format(aBuf, sizeof(aBuf), "Playlist %s could not be found!", pName);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MediaPlayer", aBuf);
		return;//could not find playlist
	}

	if(pThis->PreRemovePlaylist(PlaylistID))
	{
		str_format(aBuf, sizeof(aBuf), "Playlist %s could not be removed.", pName);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MediaPlayer", aBuf);
	}
	else
	{
		str_format(aBuf, sizeof(aBuf), "Playlist %s has been removed successfully.", pName);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MediaPlayer", aBuf);
	}
}

void CMediaPlayer::ConMediaTrackAdd(IConsole::IResult *pResult, void *pUserData)
{
	CMediaPlayer *pThis = (CMediaPlayer*)pUserData;
	array<CMediaPlaylist *> pPlayLists = pThis->GetPlayLists();
	const char *pPlaylist = pResult->GetString(0);
	const char *pFileName = pResult->GetString(1);
	char aBuf[256];

	int PlaylistID = pThis->GetPlaylistID((char *)pPlaylist);
	if(PlaylistID == -1)
	{
		str_format(aBuf, sizeof(aBuf), "Playlist %s could not be found!", pPlaylist);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MediaPlayer", aBuf);
		return;//could not find playlist
	}

	bool Done = pThis->AddTrack(PlaylistID, (char *)pFileName);
	if(Done == false)
	{
		str_format(aBuf, sizeof(aBuf), "File %s could not be added.", pFileName);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MediaPlayer", aBuf);
	}
	else
	{
		str_format(aBuf, sizeof(aBuf), "File %s has been added successfully.", pFileName);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MediaPlayer", aBuf);
	}
}

void CMediaPlayer::ConMediaTrackDel(IConsole::IResult *pResult, void *pUserData)
{
	CMediaPlayer *pThis = (CMediaPlayer*)pUserData;
	array<CMediaPlaylist *> pPlayLists = pThis->GetPlayLists();
	const char *pPlaylist = pResult->GetString(0);
	const char *pFileName = pResult->GetString(1);
	char aBuf[256];

	int PlaylistID = pThis->GetPlaylistID((char *)pPlaylist);
	if(PlaylistID == -1)
	{
		str_format(aBuf, sizeof(aBuf), "Playlist %s could not be found!", pPlaylist);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MediaPlayer", aBuf);
		return;//could not find playlist
	}

	int TrackID = pThis->GetTrackID(PlaylistID, (char *)pFileName);
	if(TrackID == -1)
	{
		str_format(aBuf, sizeof(aBuf), "Track %s could not be found!", pFileName);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MediaPlayer", aBuf);
		return;//could not find playlist
	}

	if(pThis->RemoveTrack(PlaylistID, TrackID))
	{
		str_format(aBuf, sizeof(aBuf), "Track %s has been removed successfully!", pFileName);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MediaPlayer", aBuf);
	}
	else
	{
		str_format(aBuf, sizeof(aBuf), "Track %s could not be removed!", pFileName);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MediaPlayer", aBuf);
	}
}