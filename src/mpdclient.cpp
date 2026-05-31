// <one line to give the program's name and a brief idea of what it does.>
// SPDX-FileCopyrightText: %{CURRENT_YEAR} %{AUTHOR} <%{EMAIL}>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mpdclient.h"

#include <QDateTime>
#include <QDebug>

#include <variant>

#include <mpd/client.h>

// Private utility types & methods
template<class... Ts>
struct overload : Ts... {
    using Ts::operator()...;
};
template<class... Ts>
overload(Ts...) -> overload<Ts...>;

struct QScopedPointerMpdConnectionDeleter {
    static inline void cleanup(mpd_connection *pointer) noexcept
    {
        if (pointer != nullptr) {
            mpd_connection_free(pointer);
        }
    }
    void operator()(mpd_connection *pointer) const noexcept
    {
        cleanup(pointer);
    }
};

struct MpdSearchConstraintByAddedSince {
    const QDateTime value;
};

struct MpdSearchConstraintByAnyTag {
    QStringView value;
};

struct MpdSearchConstraintByBase {
    QStringView value;
};

struct MpdSearchConstraintByModifiedSince {
    const QDateTime value;
};

struct MpdSearchConstraintByTag {
    mpd_tag_type type;
    QStringView value;
};

struct MpdSearchConstraintByUri {
    QStringView value;
};

struct MpdSearchInitializationAddDbSongs {
    bool exact;
};

struct MpdSearchInitializationAddDbSongsToPlaylist {
    bool exact;
    QStringView playlistName;
};

struct MpdSearchInitializationCountDbSongs {
};

struct MpdSearchInitializationSearchDbSongs {
    bool exact;
};

struct MpdSearchPosition {
    uint position;
    mpd_position_whence whence;
};

struct MpdSearchSortBySortName {
    QStringView name;
    bool descending;
};

struct MpdSearchSortByTag {
    mpd_tag_type type;
    bool descending;
};

struct MpdSearchWindow {
    uint start;
    uint end;
};

struct mpd_connection *initConnection()
{
    struct mpd_connection *newConnection = mpd_connection_new(nullptr, 0, 0);
    if (newConnection != nullptr && mpd_connection_get_error(newConnection) != MPD_ERROR_SUCCESS) {
        qCritical() << "Could not create new MPD server connection. Reason: " << mpd_connection_get_error_message(newConnection);
        mpd_connection_clear_error(newConnection);
        mpd_connection_free(newConnection);
        newConnection = nullptr;
    }
    return newConnection;
}

bool searchSongs(mpd_connection *connection,
                 const std::variant<MpdSearchInitializationAddDbSongs,
                                    MpdSearchInitializationAddDbSongsToPlaylist,
                                    MpdSearchInitializationCountDbSongs,
                                    MpdSearchInitializationSearchDbSongs> &initializationParameters,
                 const std::variant<std::monostate,
                                    MpdSearchConstraintByAddedSince,
                                    MpdSearchConstraintByAnyTag,
                                    MpdSearchConstraintByBase,
                                    MpdSearchConstraintByModifiedSince,
                                    MpdSearchConstraintByTag,
                                    MpdSearchConstraintByUri> &searchConstraint,
                 const std::variant<std::monostate, MpdSearchSortBySortName, MpdSearchSortByTag> &sortByClause,
                 const std::optional<MpdSearchPosition> &position,
                 const std::optional<MpdSearchWindow> &window)
{
    if (!std::visit(overload{
                        [connection](const MpdSearchInitializationAddDbSongs &addToQueue) {
                            return mpd_search_add_db_songs(connection, addToQueue.exact);
                        },
                        [connection](const MpdSearchInitializationAddDbSongsToPlaylist &addToPlaylist) {
                            return mpd_search_add_db_songs_to_playlist(connection, addToPlaylist.playlistName.toUtf8().constData());
                        },
                        [connection](const MpdSearchInitializationCountDbSongs &count) {
                            return mpd_count_db_songs(connection);
                        },
                        [connection](const MpdSearchInitializationSearchDbSongs &search) {
                            return mpd_search_db_songs(connection, search.exact);
                        },
                    },
                    initializationParameters)) {
        qWarning() << "An error occured while initializing song search: " << mpd_connection_get_error_message(connection);
        mpd_connection_clear_error(connection);
        return false;
    }

    if (!std::visit(overload{
                        [](const std::monostate &arg) {
                            return true;
                        },
                        [connection](const MpdSearchConstraintByAddedSince &byAddedSince) {
                            return mpd_search_add_added_since_constraint(connection, MPD_OPERATOR_DEFAULT, byAddedSince.value.toSecsSinceEpoch());
                        },
                        [connection](const MpdSearchConstraintByAnyTag &byAnyTag) {
                            return mpd_search_add_any_tag_constraint(connection, MPD_OPERATOR_DEFAULT, byAnyTag.value.toUtf8().constData());
                        },
                        [connection](const MpdSearchConstraintByBase &byBase) {
                            return mpd_search_add_base_constraint(connection, MPD_OPERATOR_DEFAULT, byBase.value.toUtf8().constData());
                        },
                        [connection](const MpdSearchConstraintByModifiedSince &byModifiedSince) {
                            return mpd_search_add_modified_since_constraint(connection, MPD_OPERATOR_DEFAULT, byModifiedSince.value.toSecsSinceEpoch());
                        },
                        [connection](const MpdSearchConstraintByTag &byTag) {
                            return mpd_search_add_tag_constraint(connection, MPD_OPERATOR_DEFAULT, byTag.type, byTag.value.toUtf8().constData());
                        },
                        [connection](const MpdSearchConstraintByUri &byUri) {
                            return mpd_search_add_uri_constraint(connection, MPD_OPERATOR_DEFAULT, byUri.value.toUtf8().constData());
                        },
                    },
                    searchConstraint)) {
        mpd_search_cancel(connection);
        qWarning() << "An error occured while adding song search constraint: " << mpd_connection_get_error_message(connection);
        mpd_connection_clear_error(connection);
        return false;
    }

    if (!std::visit(overload{
                        [](const std::monostate &arg) {
                            return true;
                        },
                        [connection](const MpdSearchSortBySortName &bySortName) {
                            return mpd_search_add_sort_name(connection, bySortName.name.toUtf8().constData(), bySortName.descending);
                        },
                        [connection](const MpdSearchSortByTag &byTag) {
                            return mpd_search_add_sort_tag(connection, byTag.type, byTag.descending);
                        },
                    },
                    sortByClause)) {
        mpd_search_cancel(connection);
        qWarning() << "An error occured while adding songs sort clause: " << mpd_connection_get_error_message(connection);
        mpd_connection_clear_error(connection);
        return false;
    }
    if (position.has_value()) {
        const MpdSearchPosition &positionParameters = position.value();
        if (!mpd_search_add_position(connection, positionParameters.position, positionParameters.whence)) {
            mpd_search_cancel(connection);
            qWarning() << "An error occured while adding songs search position clause: " << mpd_connection_get_error_message(connection);
            mpd_connection_clear_error(connection);
            return false;
        }
    }
    if (window.has_value()) {
        const MpdSearchWindow &windowParameters = window.value();
        if (!mpd_search_add_window(connection, windowParameters.start, windowParameters.end)) {
            mpd_search_cancel(connection);
            qWarning() << "An error occured while adding songs search window: " << mpd_connection_get_error_message(connection);
            mpd_connection_clear_error(connection);
            return false;
        }
    }

    bool success = mpd_search_commit(connection);
    if (!success) {
        qWarning() << "An error occured while starting song search: " << mpd_connection_get_error_message(connection);
        mpd_connection_clear_error(connection);
    }
    return success;
}

void searchTags(mpd_connection *connection, const mpd_tag_type requestedTag)
{
    if (!mpd_search_db_tags(connection, requestedTag)) {
        qWarning() << "An error occured while initializing tags search: " << mpd_connection_get_error_message(connection);
        mpd_connection_clear_error(connection);
        return;
    }
    if (!mpd_search_commit(connection)) {
        qWarning() << "An error occured while starting tags search: " << mpd_connection_get_error_message(connection);
        mpd_connection_clear_error(connection);
        return;
    }

    for (struct mpd_pair *tag = mpd_recv_pair_tag(connection, requestedTag); tag != nullptr;) {
        qDebug() << tag->value;
        mpd_return_pair(connection, tag);
    }

    if (mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS || !mpd_response_finish(connection)) {
        qWarning() << "An error occured while searching tags: " << mpd_connection_get_error_message(connection);
        mpd_connection_clear_error(connection);
    }
}

// Main class body
MpdClient::MpdClient()
    : idleChannel(QSocketNotifier(QSocketNotifier::Read))
{
    connect(&idleChannel, &QSocketNotifier::activated, this, [this](QSocketDescriptor socket, QSocketNotifier::Type eventType) {
        if (eventType == QSocketNotifier::Read && socket.isValid()) {
            enum mpd_idle events = mpd_recv_idle(this->idleMpdConnection.data(), false);
            if (events == 0 && mpd_connection_get_error(this->idleMpdConnection.data()) != MPD_ERROR_SUCCESS) {
                qWarning() << "Error occured on recieving events from MPD server: " << mpd_connection_get_error_message(idleMpdConnection.data());
                if (mpd_connection_clear_error(idleMpdConnection.data())) {
                    qDebug() << "Trying to resume idling…";
                    if (!mpd_send_idle(this->idleMpdConnection.data())) {
                        qWarning() << "An error occured on idling: " << mpd_connection_get_error_message(idleMpdConnection.data());
                        mpd_connection_clear_error(idleMpdConnection.data());
                        this->idleChannel.setSocket(QSocketDescriptor());
                        this->idleMpdConnection.reset();
                        this->mainMpdConnection.reset();
                    }
                } else {
                    qCritical() << "Critical error, now dropping MPD server connections";
                    this->idleChannel.setSocket(QSocketDescriptor());
                    this->idleMpdConnection.reset();
                    this->mainMpdConnection.reset();
                }
            } else {
                if (events & MPD_IDLE_DATABASE) {
                    qDebug() << "MPD: song database has been updated";
                    Q_EMIT databaseUpdated();
                }
                if (events & MPD_IDLE_STORED_PLAYLIST) {
                    qDebug() << "MPD: a stored playlist has been modified, created, deleted or renamed";
                    Q_EMIT storedPlaylistsChanged();
                }
                if (events & MPD_IDLE_QUEUE) {
                    qDebug() << "MPD: the queue has been modified";
                    Q_EMIT queueChanged();
                }
                if (events & MPD_IDLE_PLAYER) {
                    qDebug() << "MPD: the player state has changed: play, stop, pause, seek, ...";
                    Q_EMIT playerStateChanged();
                }
                if (events & MPD_IDLE_MIXER) {
                    qDebug() << "MPD: the volume has been modified";
                    Q_EMIT volumeChanged();
                }
                if (events & MPD_IDLE_OUTPUT) {
                    qDebug() << "MPD: an audio output device has been enabled or disabled";
                    Q_EMIT outputsChanged();
                }
                if (events & MPD_IDLE_OPTIONS) {
                    qDebug() << "MPD: options have changed: crossfade, random, repeat, ...";
                    Q_EMIT optionsChanged();
                }
                if (events & MPD_IDLE_UPDATE) {
                    qDebug() << "MPD: a database update has started or finished";
                    Q_EMIT databaseUpdateStateChanged();
                }
                if (events & MPD_IDLE_STICKER) {
                    qDebug() << "MPD: a sticker has been modified";
                    Q_EMIT stickersChanged();
                }
                if (events & MPD_IDLE_SUBSCRIPTION) {
                    qDebug() << "MPD: a client has subscribed to or unsubscribed from a channel";
                    Q_EMIT channelSubscriptionChanged();
                }
                if (events & MPD_IDLE_MESSAGE) {
                    qDebug() << "MPD: a message on a subscribed channel was received";
                    Q_EMIT messageReceived();
                }
                if (events & MPD_IDLE_PARTITION) {
                    qDebug() << "MPD: a partition was added or changed";
                    Q_EMIT partitionsChanged();
                }
                if (events & MPD_IDLE_NEIGHBOR) {
                    qDebug() << "MPD: a neighbor was found or lost";
                    Q_EMIT neighborsChanged();
                }
                if (events & MPD_IDLE_MOUNT) {
                    qDebug() << "MPD: the mount list has changed";
                    Q_EMIT mountsChanged();
                }

                if (!mpd_send_idle(this->idleMpdConnection.data())) {
                    qWarning() << "An error occured on idling: " << mpd_connection_get_error_message(idleMpdConnection.data());
                    mpd_connection_clear_error(idleMpdConnection.data());
                    this->idleChannel.setSocket(QSocketDescriptor());
                    this->idleMpdConnection.reset();
                    this->mainMpdConnection.reset();
                }
            }
        } else if (eventType == QSocketNotifier::Exception) {
            qWarning() << "An error occured on idle MPD socket, dropping MPD server connections";
            this->idleChannel.setSocket(QSocketDescriptor());
            this->idleMpdConnection.reset();
            this->mainMpdConnection.reset();
        }
    });
}

// Public API
bool MpdClient::addSongToPlaylist(QStringView playlistName, QStringView songUri)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_playlist_add(mainMpdConnection.data(), playlistName.toUtf8().constData(), songUri.toUtf8().constData());
    if (!success) {
        qWarning() << "An error occurred while adding song to playlist: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::clearPlaylist(QStringView playlistName)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_playlist_clear(mainMpdConnection.data(), playlistName.toUtf8().constData());
    if (!success) {
        qWarning() << "An error occurred while clearing playlist: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::changePlaybackPosition(float change)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_seek_current(mainMpdConnection.data(), change, true);
    if (!success) {
        qWarning() << "An error occurred while changing playback position: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::changeVolume(int change)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_change_volume(mainMpdConnection.data(), change);
    if (!success) {
        qWarning() << "An error occurred while changing volume: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::clearQueue()
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_clear(mainMpdConnection.data());
    if (!success) {
        qWarning() << "An error occurred while clearing queue: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::deletePlaylist(QStringView playlistName)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_rm(mainMpdConnection.data(), playlistName.toUtf8().constData());
    if (!success) {
        qWarning() << "An error occurred while deleting playlist: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::deleteSongFromPlaylist(QStringView playlistName, uint songPosition)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_playlist_delete(mainMpdConnection.data(), playlistName.toUtf8().constData(), songPosition);
    if (!success) {
        qWarning() << "An error occurred while deleting song from playlist: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::deleteSongFromQueue(uint songId)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_delete_id(mainMpdConnection.data(), songId);
    if (!success) {
        qWarning() << "An error occurred while deleting song from queue: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::deleteSongsFromQueue(const QList<uint> &songIds)
{
    if (!refreshMainConnection()) {
        return false;
    }

    mpd_command_list_begin(mainMpdConnection.data(), false);
    for (const uint &songId : songIds) {
        mpd_send_delete_id(mainMpdConnection.data(), songId);
    }
    mpd_command_list_end(mainMpdConnection.data());

    const bool success = mpd_response_finish(mainMpdConnection.data());
    if (!success) {
        qWarning() << "An error occurred while deleting songs from queue: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::disableOutput(uint outputId)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_disable_output(mainMpdConnection.data(), outputId);
    if (!success) {
        qWarning() << "An error occurred while disabling output: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::enableOutput(uint outputId)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_enable_output(mainMpdConnection.data(), outputId);
    if (!success) {
        qWarning() << "An error occurred while enabling output: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::enqeueueAlbum(QStringView album, bool playNext)
{
    if (!refreshMainConnection()) {
        return false;
    }

    return ::searchSongs(mainMpdConnection.data(),
                         MpdSearchInitializationAddDbSongs{true},
                         MpdSearchConstraintByTag{MPD_TAG_ALBUM, album},
                         std::monostate(),
                         playNext ? std::make_optional(MpdSearchPosition{0, MPD_POSITION_AFTER_CURRENT}) : std::nullopt,
                         std::nullopt);
}

bool MpdClient::enqeueueArtistDiscography(QStringView albumArtist, bool playNext)
{
    if (!refreshMainConnection()) {
        return false;
    }

    return ::searchSongs(mainMpdConnection.data(),
                         MpdSearchInitializationAddDbSongs{true},
                         MpdSearchConstraintByTag{MPD_TAG_ALBUM, albumArtist},
                         std::monostate(),
                         playNext ? std::make_optional(MpdSearchPosition{0, MPD_POSITION_AFTER_CURRENT}) : std::nullopt,
                         std::nullopt);
}

bool MpdClient::enqueuePlaylist(QStringView playlistName)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_load(mainMpdConnection.data(), playlistName.toUtf8().constData());
    if (!success) {
        qWarning() << "An error occurred while adding playlist to queue: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::enqueueSong(QStringView songUri, bool playNext)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = playNext ? mpd_run_add_whence(mainMpdConnection.data(), songUri.toUtf8().constData(), 0, MPD_POSITION_AFTER_CURRENT)
                                  : mpd_run_add(mainMpdConnection.data(), songUri.toUtf8().constData());
    if (!success) {
        qWarning() << "An error occurred while adding song to queue: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::enqueueSongs(const QList<QStringView> &songUris)
{
    if (!refreshMainConnection()) {
        return false;
    }

    mpd_command_list_begin(mainMpdConnection.data(), false);
    for (QStringView songUri : songUris) {
        mpd_send_add(mainMpdConnection.data(), songUri.toUtf8().constData());
    }
    mpd_command_list_end(mainMpdConnection.data());

    const bool success = mpd_response_finish(mainMpdConnection.data());
    if (!success) {
        qWarning() << "An error occurred while adding songs to queue: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

void MpdClient::getAlbumSongs(QStringView album)
{
    if (!refreshMainConnection()) {
        return;
    }

    if (!::searchSongs(mainMpdConnection.data(),
                       MpdSearchInitializationSearchDbSongs{true},
                       MpdSearchConstraintByTag{MPD_TAG_ALBUM, album},
                       MpdSearchSortByTag{MPD_TAG_DISC, false},
                       std::nullopt,
                       std::nullopt)) {
        return;
    }

    for (struct mpd_song *song = mpd_recv_song(mainMpdConnection.data()); song != nullptr;) {
        const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
        if (title != nullptr) {
            qDebug() << "uri = " << mpd_song_get_uri(song) << ", title = " << title << ", album = " << mpd_song_get_tag(song, MPD_TAG_ALBUM, 0)
                     << ", disc = " << mpd_song_get_tag(song, MPD_TAG_DISC, 0) << ", album position = " << mpd_song_get_tag(song, MPD_TAG_TRACK, 0)
                     << ", album artist = " << mpd_song_get_tag(song, MPD_TAG_ALBUM_ARTIST, 0) << ", artists = " << mpd_song_get_tag(song, MPD_TAG_ARTIST, 0)
                     << ", duration = " << mpd_song_get_duration(song);
        }
        mpd_song_free(song);
    }

    if (mpd_connection_get_error(mainMpdConnection.data()) != MPD_ERROR_SUCCESS || !mpd_response_finish(mainMpdConnection.data())) {
        qWarning() << "An error occured while receiving songs: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
}

void MpdClient::getArtistDiscography(QStringView albumArtist)
{
    if (!refreshMainConnection()) {
        return;
    }

    if (!::searchSongs(mainMpdConnection.data(),
                       MpdSearchInitializationSearchDbSongs{true},
                       MpdSearchConstraintByTag{MPD_TAG_ALBUM_ARTIST, albumArtist},
                       MpdSearchSortByTag{MPD_TAG_ALBUM_SORT, false},
                       std::nullopt,
                       std::nullopt)) {
        return;
    }

    for (struct mpd_song *song = mpd_recv_song(mainMpdConnection.data()); song != nullptr;) {
        const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
        if (title != nullptr) {
            qDebug() << "uri = " << mpd_song_get_uri(song) << ", title = " << title << ", album = " << mpd_song_get_tag(song, MPD_TAG_ALBUM, 0)
                     << ", disc = " << mpd_song_get_tag(song, MPD_TAG_DISC, 0) << ", album position = " << mpd_song_get_tag(song, MPD_TAG_TRACK, 0)
                     << ", album artist = " << mpd_song_get_tag(song, MPD_TAG_ALBUM_ARTIST, 0) << ", artist = " << mpd_song_get_tag(song, MPD_TAG_ARTIST, 0)
                     << ", date = " << mpd_song_get_tag(song, MPD_TAG_DATE, 0) << ", duration = " << mpd_song_get_duration(song);
        }
        mpd_song_free(song);
    }

    if (mpd_connection_get_error(mainMpdConnection.data()) != MPD_ERROR_SUCCESS || !mpd_response_finish(mainMpdConnection.data())) {
        qWarning() << "An error occured while receiving songs: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
}

void MpdClient::getLastModifiedSongs(const QDateTime &since)
{
    if (!refreshMainConnection()) {
        return;
    }

    if (!::searchSongs(mainMpdConnection.data(),
                       MpdSearchInitializationSearchDbSongs{true},
                       MpdSearchConstraintByModifiedSince{since},
                       MpdSearchSortBySortName{QStringLiteral("Last-Modified"), true},
                       std::nullopt,
                       std::nullopt)) {
        return;
    }

    for (struct mpd_song *song = mpd_recv_song(mainMpdConnection.data()); song != nullptr;) {
        const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
        if (title != nullptr) {
            qDebug() << "uri = " << mpd_song_get_uri(song) << ", title = " << title << ", album = " << mpd_song_get_tag(song, MPD_TAG_ALBUM, 0)
                     << ", artists = " << mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) << ", duration = " << mpd_song_get_duration(song);
        }
        mpd_song_free(song);
    }

    if (mpd_connection_get_error(mainMpdConnection.data()) != MPD_ERROR_SUCCESS || !mpd_response_finish(mainMpdConnection.data())) {
        qWarning() << "An error occured while receiving songs: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
}

void MpdClient::getOutputs()
{
    if (!refreshMainConnection()) {
        return;
    }

    if (!mpd_send_outputs(mainMpdConnection.data())) {
        qWarning() << "An error occured while requesting configured outputs: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
        return;
    }

    for (struct mpd_output *output = mpd_recv_output(mainMpdConnection.data()); output != nullptr;) {
        qDebug() << "id = " << mpd_output_get_id(output) << ", name = " << mpd_output_get_name(output) << ", plugin = " << mpd_output_get_plugin(output)
                 << ", enabled = " << mpd_output_get_enabled(output);
        mpd_output_free(output);
    }

    if (mpd_connection_get_error(mainMpdConnection.data()) != MPD_ERROR_SUCCESS || !mpd_response_finish(mainMpdConnection.data())) {
        qWarning() << "An error occured while receiving configured outputs: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
}

void MpdClient::getPlaybackOptions()
{
    if (!refreshMainConnection()) {
        return;
    }

    if (struct mpd_status *status = mpd_run_status(mainMpdConnection.data()); status != nullptr) {
        qDebug() << "consume = " << mpd_status_get_consume(status) << ", random = " << mpd_status_get_random(status)
                 << ", repeat = " << mpd_status_get_repeat(status) << ", single = " << mpd_status_get_single(status);
        mpd_status_free(status);
    } else {
        qWarning() << "An error occured while requesting playback options: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
}

void MpdClient::getPlaybackState()
{
    if (!refreshMainConnection()) {
        return;
    }

    if (struct mpd_status *status = mpd_run_status(mainMpdConnection.data()); status != nullptr) {
        qDebug() << "current song id = " << mpd_status_get_song_id(status) << ", elapsed seconds = " << (mpd_status_get_elapsed_ms(status) / 1000)
                 << ", total seconds = " << mpd_status_get_total_time(status) << ", has next = " << (mpd_status_get_next_song_id(status) != -1)
                 << ", has previous = " << (mpd_status_get_song_pos(status) > 0) << ", playback status = " << mpd_status_get_state(status);
        mpd_status_free(status);
    } else {
        qWarning() << "An error occured while requesting playback options: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
}

void MpdClient::getPlayerFullState()
{
    if (!refreshMainConnection()) {
        return;
    }

    if (struct mpd_status *status = mpd_run_status(mainMpdConnection.data()); status != nullptr) {
        int currentSongId = mpd_status_get_song_id(status);
        int volume = mpd_status_get_volume(status);

        if (currentSongId != -1) {
            getSongFromQueue(currentSongId);
        }

        qDebug() << "current song id = " << currentSongId << ", elapsed seconds = " << (mpd_status_get_elapsed_ms(status) / 1000)
                 << ", total seconds = " << mpd_status_get_total_time(status) << ", has next = " << (mpd_status_get_next_song_id(status) != -1)
                 << ", has previous = " << (mpd_status_get_song_pos(status) > 0) << ", playback status = " << mpd_status_get_state(status)
                 << ", consume = " << mpd_status_get_consume(status) << ", random = " << mpd_status_get_random(status)
                 << ", repeat = " << mpd_status_get_repeat(status) << ", single = " << mpd_status_get_single(status) << ", audio available = " << (volume >= 0)
                 << ", volume = " << volume;

        mpd_status_free(status);
    } else {
        qWarning() << "An error occured while requesting playback options: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
}

void MpdClient::getPlaylists()
{
    if (!refreshMainConnection()) {
        return;
    }

    if (!mpd_send_list_playlists(mainMpdConnection.data())) {
        qWarning() << "An error occured while requesting playlists: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
        return;
    }

    for (struct mpd_playlist *playlist = mpd_recv_playlist(mainMpdConnection.data()); playlist != nullptr;) {
        qDebug() << mpd_playlist_get_path(playlist) << QDateTime::fromSecsSinceEpoch(mpd_playlist_get_last_modified(playlist));
        mpd_playlist_free(playlist);
    }

    if (mpd_connection_get_error(mainMpdConnection.data()) != MPD_ERROR_SUCCESS || !mpd_response_finish(mainMpdConnection.data())) {
        qWarning() << "An error occured while searching playlists: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
}

void MpdClient::getPlaylistSongs(QStringView playlistName)
{
    if (!refreshMainConnection()) {
        return;
    }

    if (!mpd_send_list_playlist_meta(mainMpdConnection.data(), playlistName.toUtf8().constData())) {
        qWarning() << "An error occured while requesting playlist: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
        return;
    }

    for (struct mpd_entity *entity = mpd_recv_entity(mainMpdConnection.data()); entity != nullptr;) {
        if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
            const struct mpd_song *song = mpd_entity_get_song(entity);
            qDebug() << "uri = " << mpd_song_get_uri(song) << ", title = " << mpd_song_get_tag(song, MPD_TAG_TITLE, 0)
                     << ", album = " << mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) << ", artist = " << mpd_song_get_tag(song, MPD_TAG_ARTIST, 0)
                     << ", duration = " << mpd_song_get_duration(song);
        }
        mpd_entity_free(entity);
    }

    if (mpd_connection_get_error(mainMpdConnection.data()) != MPD_ERROR_SUCCESS || !mpd_response_finish(mainMpdConnection.data())) {
        qWarning() << "An error occured while receiving playlist: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
}

void MpdClient::getQueue()
{
    if (!refreshMainConnection()) {
        return;
    }

    if (!mpd_send_list_queue_meta(mainMpdConnection.data())) {
        qWarning() << "An error occured while requesting queue: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
        return;
    }

    for (struct mpd_entity *entity = mpd_recv_entity(mainMpdConnection.data()); entity != nullptr;) {
        if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
            const struct mpd_song *song = mpd_entity_get_song(entity);
            qDebug() << "id = " << mpd_song_get_id(song) << ", position = " << mpd_song_get_pos(song) << ", uri = " << mpd_song_get_uri(song)
                     << ", title = " << mpd_song_get_tag(song, MPD_TAG_TITLE, 0) << ", album = " << mpd_song_get_tag(song, MPD_TAG_ALBUM, 0)
                     << ", artist = " << mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) << ", duration = " << mpd_song_get_duration(song);
        }
        mpd_entity_free(entity);
    }

    if (mpd_connection_get_error(mainMpdConnection.data()) != MPD_ERROR_SUCCESS || !mpd_response_finish(mainMpdConnection.data())) {
        qWarning() << "An error occured while receiving queue: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
}

void MpdClient::getSongDetails(QStringView songUri)
{
    if (!refreshMainConnection()) {
        return;
    }

    if (!::searchSongs(mainMpdConnection.data(),
                       MpdSearchInitializationSearchDbSongs{true},
                       MpdSearchConstraintByUri{songUri},
                       std::monostate(),
                       std::nullopt,
                       std::nullopt)) {
        return;
    }

    for (struct mpd_song *song = mpd_recv_song(mainMpdConnection.data()); song != nullptr;) {
        if (const struct mpd_audio_format *audioFormat = mpd_song_get_audio_format(song); audioFormat != nullptr) {
            qDebug() << "bits = " << audioFormat->bits << ", channels = " << audioFormat->channels << ", sample rate = " << audioFormat->sample_rate;
        }

        const char *dateValue = mpd_song_get_tag(song, MPD_TAG_DATE, 0);
        const QDateTime date = dateValue == nullptr ? QDateTime() : QDateTime::fromString(QLatin1StringView(dateValue), Qt::ISODate);

        const char *originalDateValue = mpd_song_get_tag(song, MPD_TAG_ORIGINAL_DATE, 0);
        const QDateTime originalDate = originalDateValue == nullptr ? QDateTime() : QDateTime::fromString(QLatin1StringView(dateValue), Qt::ISODate);

        qDebug() << "album = " << mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) << ", album artist = " << mpd_song_get_tag(song, MPD_TAG_ALBUM_ARTIST, 0)
                 << ", artists = " << mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) << ", comment = " << mpd_song_get_tag(song, MPD_TAG_COMMENT, 0)
                 << ", composers = " << mpd_song_get_tag(song, MPD_TAG_COMPOSER, 0) << ", conductors = " << mpd_song_get_tag(song, MPD_TAG_CONDUCTOR, 0)
                 << ", date = " << date << ", date value = " << dateValue << ", disc = " << mpd_song_get_tag(song, MPD_TAG_DISC, 0)
                 << ", ensemble = " << mpd_song_get_tag(song, MPD_TAG_ENSEMBLE, 0) << ", genre = " << mpd_song_get_tag(song, MPD_TAG_GENRE, 0)
                 << ", grouping = " << mpd_song_get_tag(song, MPD_TAG_GROUPING, 0) << ", location = " << mpd_song_get_tag(song, MPD_TAG_LOCATION, 0)
                 << ", mood = " << mpd_song_get_tag(song, MPD_TAG_MOOD, 0) << ", movement = " << mpd_song_get_tag(song, MPD_TAG_MOVEMENT, 0)
                 << ", movement number = " << mpd_song_get_tag(song, MPD_TAG_MOVEMENTNUMBER, 0)
                 << ", musicbrainz album id = " << mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ALBUMID, 0)
                 << ", musicbrainz album artist id = " << mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ALBUMARTISTID, 0)
                 << ", musicbrainz artist id = " << mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ARTISTID, 0)
                 << ", musicbrainz release group id = " << mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_RELEASEGROUPID, 0)
                 << ", musicbrainz release track id = " << mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_RELEASETRACKID, 0)
                 << ", musicbrainz track id = " << mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_TRACKID, 0)
                 << ", musicbrainz work id = " << mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_WORKID, 0)
                 << ", name = " << mpd_song_get_tag(song, MPD_TAG_NAME, 0) << ", original date = " << originalDate
                 << ", original date value = " << originalDateValue << ", performers = " << mpd_song_get_tag(song, MPD_TAG_PERFORMER, 0)
                 << ", position = " << mpd_song_get_tag(song, MPD_TAG_TRACK, 0) << ", publisher = " << mpd_song_get_tag(song, MPD_TAG_LABEL, 0)
                 << ", title = " << mpd_song_get_tag(song, MPD_TAG_TITLE, 0) << ", track = " << mpd_song_get_tag(song, MPD_TAG_TRACK, 0)
                 << ", uri = " << mpd_song_get_uri(song) << ", work = " << mpd_song_get_tag(song, MPD_TAG_WORK, 0);
        mpd_song_free(song);
    }

    if (mpd_connection_get_error(mainMpdConnection.data()) != MPD_ERROR_SUCCESS || !mpd_response_finish(mainMpdConnection.data())) {
        qWarning() << "An error occured while receiving song details: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
}

void MpdClient::getSongFromQueue(uint id)
{
    if (!refreshMainConnection()) {
        return;
    }

    if (struct mpd_song *song = mpd_run_get_queue_song_id(mainMpdConnection.data(), id); song != nullptr) {
        qDebug() << "id = " << mpd_song_get_id(song) << ", position = " << mpd_song_get_pos(song) << ", uri = " << mpd_song_get_uri(song)
                 << ", title = " << mpd_song_get_tag(song, MPD_TAG_TITLE, 0) << ", album = " << mpd_song_get_tag(song, MPD_TAG_ALBUM, 0)
                 << ", artist = " << mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) << ", duration = " << mpd_song_get_duration(song);
        mpd_song_free(song);
    } else {
        qWarning() << "An error occured while requesting song from queue: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
}

void MpdClient::getStatistics()
{
    if (!refreshMainConnection()) {
        return;
    }

    if (struct mpd_stats *statistics = mpd_run_stats(mainMpdConnection.data()); statistics != nullptr) {
        qDebug() << "artists count = " << mpd_stats_get_number_of_artists(statistics) << ", albums count = " << mpd_stats_get_number_of_albums(statistics)
                 << ", tracks count = " << mpd_stats_get_number_of_songs(statistics) << ", MPD uptime = " << mpd_stats_get_uptime(statistics)
                 << ", DB play time = " << mpd_stats_get_db_play_time(statistics) << ", DB last update = " << mpd_stats_get_db_update_time(statistics)
                 << ", MPD play time = " << mpd_stats_get_play_time(statistics);
        mpd_stats_free(statistics);
    } else {
        qWarning() << "An error occured while requesting statistics: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
}

std::optional<uint> MpdClient::getVolume()
{
    if (!refreshMainConnection()) {
        return std::nullopt;
    }

    if (int response = mpd_run_get_volume(mainMpdConnection.data());
        response == -1 && mpd_connection_get_error(mainMpdConnection.data()) != MPD_ERROR_SUCCESS) {
        qWarning() << "An error occured while requesting current volume: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
        return std::nullopt;
    } else {
        return response >= 0 ? std::make_optional(response) : std::nullopt;
    }
}

bool MpdClient::initializeConnection()
{
    mainMpdConnection.reset(initConnection());
    if (mainMpdConnection.isNull()) {
        qCritical() << "Could not initialize main MPD server connection";
        return false;
    }

    idleMpdConnection.reset(initConnection());

    if (idleMpdConnection.isNull()) {
        qCritical() << "Could not initialize idle MPD server connection, dropping main connection";
        mainMpdConnection.reset();
    } else if (!mpd_send_idle(idleMpdConnection.data())) {
        qCritical() << "Could not start idling, dropping MPD server connections. Reason: " << mpd_connection_get_error_message(idleMpdConnection.data());
        mpd_connection_clear_error(idleMpdConnection.data());
        idleMpdConnection.reset();
        mainMpdConnection.reset();
    } else {
        idleChannel.setSocket(mpd_connection_get_fd(idleMpdConnection.data()));
        idleChannel.setEnabled(true);

        qInfo() << "MPD connection initialized";

        return true;
    }

    return false;
}

bool MpdClient::pausePlayback()
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_pause(mainMpdConnection.data(), true);
    if (!success) {
        qWarning() << "An error occurred while pausing playback: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::play()
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_play(mainMpdConnection.data());
    if (!success) {
        qWarning() << "An error occurred while starting playback: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::playAlbum(QStringView album)
{
    if (!refreshMainConnection()) {
        return false;
    }

    if (!mpd_run_clear(mainMpdConnection.data())) {
        qWarning() << "An error occurred while clearing queue: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }

    return ::searchSongs(mainMpdConnection.data(),
                         MpdSearchInitializationAddDbSongs{true},
                         MpdSearchConstraintByTag{MPD_TAG_ALBUM, album},
                         std::monostate(),
                         std::nullopt,
                         std::nullopt);
}

bool MpdClient::playArtistDiscography(QStringView albumArtist)
{
    if (!refreshMainConnection()) {
        return false;
    }

    if (!mpd_run_clear(mainMpdConnection.data())) {
        qWarning() << "An error occurred while clearing queue: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }

    return ::searchSongs(mainMpdConnection.data(),
                         MpdSearchInitializationAddDbSongs{true},
                         MpdSearchConstraintByTag{MPD_TAG_ALBUM_ARTIST, albumArtist},
                         std::monostate(),
                         std::nullopt,
                         std::nullopt);
}

bool MpdClient::playNext()
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_next(mainMpdConnection.data());
    if (!success) {
        qWarning() << "An error occurred while switching to next song: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::playPrevious()
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_previous(mainMpdConnection.data());
    if (!success) {
        qWarning() << "An error occurred while switching to previous song: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::playPlaylist(QStringView playlistName)
{
    if (!refreshMainConnection()) {
        return false;
    }

    mpd_command_list_begin(mainMpdConnection.data(), false);
    mpd_send_clear(mainMpdConnection.data());
    mpd_send_load(mainMpdConnection.data(), playlistName.toUtf8().constData());
    mpd_send_play(mainMpdConnection.data());
    mpd_command_list_end(mainMpdConnection.data());

    const bool success = mpd_response_finish(mainMpdConnection.data());
    if (!success) {
        qWarning() << "An error occurred while starting playlist playback: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::playSong(uint songId)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_play_id(mainMpdConnection.data(), songId);
    if (!success) {
        qWarning() << "An error occurred while switching to queued song: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::rescan()
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_rescan(mainMpdConnection.data(), nullptr);
    if (!success) {
        qWarning() << "An error occurred on music storage rescan request: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::renamePlaylist(QStringView oldPlaylistName, QStringView newPlaylistName)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_rename(mainMpdConnection.data(), oldPlaylistName.toUtf8().constData(), newPlaylistName.toUtf8().constData());
    if (!success) {
        qWarning() << "An error occurred while renaming playlist: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::resumePlayback()
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_pause(mainMpdConnection.data(), false);
    if (!success) {
        qWarning() << "An error occurred while resuming playback: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::saveQueueAsPlaylist(QStringView playlistName)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_save(mainMpdConnection.data(), playlistName.toUtf8().constData());
    if (!success) {
        qWarning() << "An error occurred while saving queue to playlist '" << playlistName
                   << "': " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::scan()
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_update(mainMpdConnection.data(), nullptr);
    if (!success) {
        qWarning() << "An error occurred on music storage update request: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

void MpdClient::searchAlbumArtists()
{
    if (!refreshMainConnection()) {
        return;
    }

    searchTags(mainMpdConnection.data(), MPD_TAG_ALBUM_ARTIST);
}

void MpdClient::searchAlbums()
{
    if (!refreshMainConnection()) {
        return;
    }

    searchTags(mainMpdConnection.data(), MPD_TAG_ALBUM);
}

void MpdClient::searchArtists()
{
    if (!refreshMainConnection()) {
        return;
    }

    searchTags(mainMpdConnection.data(), MPD_TAG_ARTIST);
}

void MpdClient::searchGenres()
{
    if (!refreshMainConnection()) {
        return;
    }

    searchTags(mainMpdConnection.data(), MPD_TAG_GENRE);
}

void MpdClient::searchSongs(QStringView searchQuery)
{
    if (!refreshMainConnection()) {
        return;
    }

    if (!::searchSongs(mainMpdConnection.data(),
                       MpdSearchInitializationSearchDbSongs{true},
                       MpdSearchConstraintByAnyTag{searchQuery},
                       MpdSearchSortByTag{MPD_TAG_TITLE, false},
                       std::nullopt,
                       std::nullopt)) {
        return;
    }

    for (struct mpd_song *song = mpd_recv_song(mainMpdConnection.data()); song != nullptr;) {
        if (const struct mpd_audio_format *audioFormat = mpd_song_get_audio_format(song); audioFormat != nullptr) {
            qDebug() << "bits = " << audioFormat->bits << ", channels = " << audioFormat->channels << ", sample rate = " << audioFormat->sample_rate;
        }

        const char *dateValue = mpd_song_get_tag(song, MPD_TAG_DATE, 0);
        const QDateTime date = dateValue == nullptr ? QDateTime() : QDateTime::fromString(QLatin1StringView(dateValue), Qt::ISODate);

        const char *originalDateValue = mpd_song_get_tag(song, MPD_TAG_ORIGINAL_DATE, 0);
        const QDateTime originalDate = originalDateValue == nullptr ? QDateTime() : QDateTime::fromString(QLatin1StringView(dateValue), Qt::ISODate);

        qDebug() << "uri = " << mpd_song_get_uri(song) << ", title = " << mpd_song_get_tag(song, MPD_TAG_TITLE, 0)
                 << ", album = " << mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) << ", artists = " << mpd_song_get_tag(song, MPD_TAG_ARTIST, 0)
                 << ", duration = " << mpd_song_get_duration(song);
        mpd_song_free(song);
    }

    if (mpd_connection_get_error(mainMpdConnection.data()) != MPD_ERROR_SUCCESS || !mpd_response_finish(mainMpdConnection.data())) {
        qWarning() << "An error occured while receiving songs: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
}

bool MpdClient::setConsume(bool newValue)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_consume(mainMpdConnection.data(), newValue);
    if (!success) {
        qWarning() << "An error occurred while changing consume mode: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::setRandomOrder(bool newValue)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_random(mainMpdConnection.data(), newValue);
    if (!success) {
        qWarning() << "An error occurred while changing random order mode: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::setRepeat(bool newValue)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_repeat(mainMpdConnection.data(), newValue);
    if (!success) {
        qWarning() << "An error occurred while changing repeat mode: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::setSingle(bool newValue)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_single(mainMpdConnection.data(), newValue);
    if (!success) {
        qWarning() << "An error occurred while changing single mode: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::setVolume(uint volume)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_set_volume(mainMpdConnection.data(), volume);
    if (!success) {
        qWarning() << "An error occurred while setting volume: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::swapQueuedSongs(uint songId, uint otherSongId)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_swap_id(mainMpdConnection.data(), songId, otherSongId);
    if (!success) {
        qWarning() << "An error occurred while swapping queued songs: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

bool MpdClient::toggleOutput(uint outputId)
{
    if (!refreshMainConnection()) {
        return false;
    }

    const bool success = mpd_run_toggle_output(mainMpdConnection.data(), outputId);
    if (!success) {
        qWarning() << "An error occurred while toggling output state: " << mpd_connection_get_error_message(mainMpdConnection.data());
        mpd_connection_clear_error(mainMpdConnection.data());
    }
    return success;
}

// Private class methods
bool MpdClient::refreshMainConnection()
{
    if (!mpd_send_command(mainMpdConnection.data(), "ping", NULL) || !mpd_response_finish(mainMpdConnection.data())) {
        mainMpdConnection.reset(initConnection());
        if (mainMpdConnection.isNull()) {
            this->idleChannel.setSocket(QSocketDescriptor());
            this->idleMpdConnection.reset();
            return false;
        }
    }

    return true;
}
