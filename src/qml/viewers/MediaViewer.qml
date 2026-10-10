// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtMultimedia
import org.kde.kirigami as Kirigami
import io.github.iamandelib.kpeek

// Video and audio, played as soon as the preview opens.
Rectangle {
    id: root

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
    Keys.onPressed: event => {
        if (event.key === Qt.Key_K) {
            root.togglePlayback();
            event.accepted = true;
        }
    }

    MediaPlayer {
        id: player
        source: PeekController.url
        videoOutput: video
        audioOutput: AudioOutput {
            id: audio
        }
        autoPlay: true
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
                    source: PeekController.iconName
                    fallback: "audio-x-generic"
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: Kirigami.Units.iconSizes.enormous
                    Layout.preferredHeight: Kirigami.Units.iconSizes.enormous
                }
                Kirigami.Heading {
                    readonly property string title: player.metaData.stringValue(MediaMetaData.Title)
                    text: title.length > 0 ? title : PeekController.fileName
                    level: 2
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    text: [player.metaData.stringValue(MediaMetaData.ContributingArtist), player.metaData.stringValue(MediaMetaData.AlbumTitle)].filter(s => s.length > 0).join(" — ")
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
                    text: player.playbackState === MediaPlayer.PlayingState ? i18nc("@action:button", "Pause (K)") : i18nc("@action:button", "Play (K)")
                    onClicked: root.togglePlayback()
                    QQC2.ToolTip.text: text
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
                    onClicked: audio.muted = !audio.muted
                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered
                }
            }
        }
    }
}
