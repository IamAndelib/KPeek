// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtMultimedia
import org.kde.kirigami as Kirigami
import io.github.iamandelib.kpeek

/**
 * Video and audio. Decoding runs on the GPU where it can (QtMultimedia's FFmpeg backend uses VA-API),
 * so previews normally just play. Dolphin's cached thumbnail stands in as a poster until the first frame.
 *
 * A large video in a codec GPUs commonly can't decode (e.g. 4K AV1) would be decoded on the CPU, which
 * is heavy for a quick look: it waits behind a Play button instead of starting by itself.
 */
Rectangle {
    id: root

    readonly property bool hasThumbnail: PeekController.thumbnail.toString() !== ""
    // Whether this file waits for Play instead of starting by itself; decided once its metadata has loaded.
    property bool waitsForPlay: false

    function heavyForCpu() {
        const resolution = player.metaData.value(MediaMetaData.Resolution);
        const codec = player.metaData.value(MediaMetaData.VideoCodec);
        const large = resolution !== undefined && resolution.width * resolution.height > 1920 * 1080;
        // H.264, HEVC and VP9 have been GPU-decoded on Intel, AMD and NVIDIA for years; AV1 only recently.
        const gpuFriendly = [MediaFormat.VideoCodec.H264, MediaFormat.VideoCodec.H265, MediaFormat.VideoCodec.VP9];
        return player.hasVideo && large && !gpuFriendly.includes(codec);
    }

    function formatTime(ms) {
        const total = Math.floor(ms / 1000);
        const h = Math.floor(total / 3600);
        const m = Math.floor(total % 3600 / 60);
        const s = String(total % 60).padStart(2, "0");
        return h > 0 ? `${h}:${String(m).padStart(2, "0")}:${s}` : `${m}:${s}`;
    }

    function togglePlayback() {
        player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play();
    }

    color: player.hasVideo ? "black" : Kirigami.Theme.backgroundColor
    focus: true
    // YouTube's keys: K play/pause, J/L back/forward 10 s, M mute. (Left/Right switch files.)
    Keys.onPressed: event => {
        switch (event.key) {
        case Qt.Key_K: root.togglePlayback(); break;
        case Qt.Key_J: player.position = Math.max(0, player.position - 10000); break;
        case Qt.Key_L: player.position = Math.min(player.duration, player.position + 10000); break;
        case Qt.Key_M: audio.muted = !audio.muted; break;
        default: return;
        }
        event.accepted = true;
    }

    MediaPlayer {
        id: player
        source: PeekController.url
        videoOutput: video
        audioOutput: AudioOutput {
            id: audio
        }
        onMediaStatusChanged: {
            if (mediaStatus === MediaPlayer.LoadedMedia) {
                root.waitsForPlay = root.heavyForCpu();
                if (!root.waitsForPlay) {
                    play();
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            VideoOutput {
                id: video
                anchors.fill: parent
                visible: player.hasVideo
            }

            // Poster until the first frame, so the preview never opens on a black rectangle.
            Image {
                anchors.fill: parent
                visible: root.hasThumbnail && player.position === 0 && (player.hasVideo || player.mediaStatus === MediaPlayer.LoadingMedia)
                source: PeekController.thumbnail
                fillMode: Image.PreserveAspectFit
                asynchronous: true
            }

            QQC2.RoundButton {
                anchors.centerIn: parent
                z: 1 // above the click-to-pause area
                visible: root.waitsForPlay && player.playbackState !== MediaPlayer.PlayingState && player.position === 0
                width: Kirigami.Units.gridUnit * 4
                height: width
                icon.name: "media-playback-start"
                icon.width: Kirigami.Units.iconSizes.large
                icon.height: Kirigami.Units.iconSizes.large
                text: i18nc("@action:button", "Play")
                display: QQC2.AbstractButton.IconOnly
                focusPolicy: Qt.NoFocus
                onClicked: player.play()
                QQC2.ToolTip.text: i18nc("@info:tooltip", "Large video that your graphics card may not decode: playing it uses the processor")
                QQC2.ToolTip.visible: hovered
            }

            MouseArea {
                anchors.fill: parent
                enabled: player.hasVideo
                onClicked: root.togglePlayback()
            }

            // Audio files: icon and tags instead of a picture.
            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(parent.width - Kirigami.Units.gridUnit * 4, Kirigami.Units.gridUnit * 24)
                visible: !player.hasVideo && player.error === MediaPlayer.NoError
                spacing: Kirigami.Units.largeSpacing

                Kirigami.Icon {
                    // Cover art from the thumbnail cache when there is one.
                    source: root.hasThumbnail ? PeekController.thumbnail : PeekController.iconName
                    fallback: "audio-x-generic"
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: Kirigami.Units.iconSizes.enormous
                    Layout.preferredHeight: Kirigami.Units.iconSizes.enormous
                }
                Kirigami.Heading {
                    readonly property string title: player.metaData.stringValue(MediaMetaData.Title)
                    text: title.length > 0 ? title : PeekController.fileName
                    textFormat: Text.PlainText // tags come from the file: never render them as markup
                    level: 2
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    text: [player.metaData.stringValue(MediaMetaData.ContributingArtist), player.metaData.stringValue(MediaMetaData.AlbumTitle)].filter(s => s.length > 0).join(" — ")
                    textFormat: Text.PlainText
                    visible: text.length > 0
                    opacity: 0.7
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
            }

            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.gridUnit * 4
                visible: player.error !== MediaPlayer.NoError
                icon.name: "dialog-error"
                text: i18nc("@info", "This file could not be played")
                explanation: player.errorString
            }
        }

        QQC2.ToolBar {
            Layout.fillWidth: true
            position: QQC2.ToolBar.Footer

            RowLayout {
                anchors.fill: parent
                spacing: Kirigami.Units.smallSpacing

                QQC2.ToolButton {
                    icon.name: player.playbackState === MediaPlayer.PlayingState ? "media-playback-pause" : "media-playback-start"
                    display: QQC2.AbstractButton.IconOnly
                    text: player.playbackState === MediaPlayer.PlayingState ? i18nc("@action:button", "Pause") : i18nc("@action:button", "Play")
                    focusPolicy: Qt.NoFocus
                    onClicked: root.togglePlayback()
                    QQC2.ToolTip.text: i18nc("@info:tooltip", "%1 (K)", text)
                    QQC2.ToolTip.visible: hovered
                }
                QQC2.Label {
                    text: root.formatTime(player.position)
                    font.features: { "tnum": 1 }
                }
                QQC2.Slider {
                    Layout.fillWidth: true
                    from: 0
                    to: Math.max(1, player.duration)
                    // Don't fight the user while they drag the handle.
                    value: pressed ? value : player.position
                    enabled: player.seekable
                    focusPolicy: Qt.NoFocus
                    onMoved: player.position = value
                }
                QQC2.Label {
                    text: root.formatTime(player.duration)
                    font.features: { "tnum": 1 }
                }
                QQC2.ToolButton {
                    icon.name: audio.muted ? "audio-volume-muted" : "audio-volume-high"
                    display: QQC2.AbstractButton.IconOnly
                    text: audio.muted ? i18nc("@action:button", "Unmute") : i18nc("@action:button", "Mute")
                    focusPolicy: Qt.NoFocus
                    onClicked: audio.muted = !audio.muted
                    QQC2.ToolTip.text: i18nc("@info:tooltip", "%1 (M)", text)
                    QQC2.ToolTip.visible: hovered
                }
            }
        }
    }
}
