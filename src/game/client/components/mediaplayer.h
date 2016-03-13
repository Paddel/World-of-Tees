
#include <game/client/components/menus.h>

struct CMediaAddEntry
{
	bool m_Folder;
	char m_aName[256];
	CMediaAddEntry(char *pName = "-", bool Folder = false)// allow standart constructor
	{
		str_copy(m_aName, pName, sizeof(m_aName));
		m_Folder = Folder;
	}

	bool operator<(const CMediaAddEntry &Other) { return (m_Folder == Other.m_Folder? str_comp_nocase(m_aName, Other.m_aName) < 0 : m_Folder > Other.m_Folder); }
};

struct CMediaFile
{
	char m_aName[256];
	char m_aFileName[256];
	bool m_NotFound;

	CMediaFile(char *pFileName);
};

struct CMediaPlaylist
{
	array<CMediaFile *> m_pFiles;
	char m_aName[64];
	bool m_Default;

	CMediaPlaylist(char *pName);
	CMediaPlaylist();
};

class CMediaPlayer : public CComponent
{
public:
	enum
	{
		ACTION_NONE=0,
		ACTION_PLAYLIST_REMOVE,
		ACTION_PLAYLIST_ADD,
		ACTION_TRACK_ADD,
	};

private:
	array<CMediaPlaylist *> m_pPlaylists;
	int m_SelectedPlaylist;
	int m_SelectedTrack;
	int m_PlayingPlaylist;
	int m_PlayingTrack;
	char m_aCurrentTrack[256];
	int m_Action;
	int m_RemovingPlaylist;
	int m_AddingPlaylistTrack;
	bool m_TrackPaused;
	int m_Loop;

	bool ReadMediaFile();
	bool SaveMediaFile();

public:
	CMediaPlayer();

	sorted_array<CMediaAddEntry> m_AddEntries;

	virtual void OnInit();
	virtual void OnShutdown();
	virtual void OnConsoleInit();
	virtual void OnTick();

	bool Play(char *pFileName);
	void Pause();
	void Resume();
	void Stop();
	void SetVolume(int NewVolume);
	void SetPos(int Position);
	int GetPos();
	int GetLength();
	bool IsPlaying();

	int GetPlaylistID(char *pName);
	int GetTrackID(int PlaylistID, char *pName);
	void PreAddPlaylist();
	bool AddPlaylist(char *pName, bool Save = true);
	bool PreRemovePlaylist(int PlaylistID);
	bool RemovePlaylist(int PlaylistID);
	void PlayPlaylist(int Index);
	void PlayTrack(int Playlist, int Index);
	void PreAddTrack(int Playlist);
	bool AddTrack(int PlaylistID, char *pFileName, bool Save = true);
	bool RemoveTrack(int PlaylistID, int Index);
	bool NextTrack();
	bool PrevTrack();
	void CheckTrackFound(CMediaFile *pTrack);
	CMediaFile *PlayingTrack();
	void TogglePause();

	array<CMediaPlaylist *> GetPlayLists() { return m_pPlaylists; }
	void SetSelectedPlaylist(int Index);
	void SetSelectedTrack(int Index) { m_SelectedTrack = Index; }
	int GetSelectedPlaylist() { return m_SelectedPlaylist; }
	int GetSelectedTrack() { return m_SelectedTrack; }
	int GetPlaylingPlaylist()  { return m_PlayingPlaylist; }
	int GetPlaylingTrack()  { return m_PlayingTrack; }
	int GetAddingPlaylistTrack() { return m_AddingPlaylistTrack; }

	//needs acces from CMenu
	int GetAction() const { return m_Action; }
	void SetAction(int NewAction) { m_Action = NewAction; }
	int GetRemovingPlaylist() const { return m_RemovingPlaylist; }
	bool TrackPaused() const { return m_TrackPaused; }
	int *Loop() { return &m_Loop; }

	static void ConMediaPlay(IConsole::IResult *pResult, void *pUserData);
	static void ConMediaVolumeSet(IConsole::IResult *pResult, void *pUserData);
	static void ConMediaPosGet(IConsole::IResult *pResult, void *pUserData);
	static void ConMediaPosSet(IConsole::IResult *pResult, void *pUserData);
	static void ConMediaPlaylistAdd(IConsole::IResult *pResult, void *pUserData);
	static void ConMediaPlaylistDel(IConsole::IResult *pResult, void *pUserData);
	static void ConMediaTrackAdd(IConsole::IResult *pResult, void *pUserData);
	static void ConMediaTrackDel(IConsole::IResult *pResult, void *pUserData);
};