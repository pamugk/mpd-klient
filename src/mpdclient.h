// <one line to give the program's name and a brief idea of what it does.>
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef MPDCLIENT_H
#define MPDCLIENT_H

#include <optional>

#include <QObject>
#include <QScopedPointer>
#include <QSocketNotifier>
#include <QVariant>

struct mpd_connection;

struct QScopedPointerMpdConnectionDeleter;

class MpdClient : public QObject
{
    Q_OBJECT

public:
    MpdClient();

    bool addSongToPlaylist(QStringView playlistName, QStringView songUri);
    bool changePlaybackPosition(float change);
    bool changeVolume(int change);
    bool clearPlaylist(QStringView playlistName);
    bool clearQueue();
    bool deletePlaylist(QStringView playlistName);
    bool deleteSongFromPlaylist(QStringView playlistName, uint songPosition);
    bool deleteSongFromQueue(uint songId);
    bool deleteSongsFromQueue(const QList<uint> &songIds);
    bool disableOutput(uint outputId);
    bool enableOutput(uint outputId);
    bool enqeueueAlbum(QStringView album, bool playNext);
    bool enqeueueArtistDiscography(QStringView albumArtist, bool playNext);
    bool enqueuePlaylist(QStringView playlistName);
    bool enqueueSong(QStringView songUri, bool playNext);
    bool enqueueSongs(const QList<QStringView> &songUris);
    void getAlbumSongs(QStringView album);
    void getArtistDiscography(QStringView albumArtist);
    void getLastModifiedSongs(const QDateTime &since);
    void getOutputs();
    void getPlaybackOptions();
    void getPlaybackState();
    void getPlayerFullState();
    void getPlaylists();
    void getPlaylistSongs(QStringView playlistName);
    void getQueue();
    void getSongDetails(QStringView songUri);
    void getSongFromQueue(uint id);
    void getStatistics();
    std::optional<uint> getVolume();
    bool initializeConnection();
    bool pausePlayback();
    bool play();
    bool playAlbum(QStringView album);
    bool playArtistDiscography(QStringView albumArtist);
    bool playNext();
    bool playPrevious();
    bool playPlaylist(QStringView playlistName);
    bool playSong(uint songId);
    bool renamePlaylist(QStringView oldPlaylistName, QStringView newPlaylistName);
    bool rescan();
    bool resumePlayback();
    bool saveQueueAsPlaylist(QStringView playlistName);
    bool scan();
    void searchAlbumArtists();
    void searchAlbums();
    void searchArtists();
    void searchGenres();
    void searchSongs(QStringView searchQuery);
    bool setConsume(bool newValue);
    bool setPlaybackPosition(float newPosition);
    bool setRandomOrder(bool newValue);
    bool setRepeat(bool newValue);
    bool setSingle(bool newValue);
    bool setVolume(uint volume);
    bool swapQueuedSongs(uint songId, uint otherSongId);
    bool toggleOutput(uint outputId);

Q_SIGNALS:
    void channelSubscriptionChanged();
    void databaseUpdated();
    void databaseUpdateStateChanged();
    void messageReceived();
    void mountsChanged();
    void neighborsChanged();
    void optionsChanged();
    void outputsChanged();
    void partitionsChanged();
    void playerStateChanged();
    void queueChanged();
    void stickersChanged();
    void storedPlaylistsChanged();
    void volumeChanged();

private:
    QScopedPointer<mpd_connection, QScopedPointerPodDeleter> mainMpdConnection;
    QScopedPointer<mpd_connection, QScopedPointerPodDeleter> idleMpdConnection;
    QSocketNotifier idleChannel;

    bool refreshMainConnection();
};

#endif // MPDCLIENT_H
