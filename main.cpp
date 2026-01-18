#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

#include <gst/gst.h>
#include <gst/sdp/sdp.h>
#include <gst/rtp/rtp.h>

#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>

#include <iostream>
#include <cassert>
#define STUN_SERVER " stun-server=stun://stun.l.google.com:19302 "
#define PIPE_LINE_NAME "sendrecv"
#define RTP_CAPS_H264 "application/x-rtp,media=video,encoding-name=H264,payload="
#define RTP_CAPS_OPUS "application/x-rtp,media=audio,encoding-name=OPUS,payload="

std::string target_url;

static GMainLoop *loop;
static GstElement *pipe1, *webrtc1 = nullptr;
static GObject *send_channel, *receive_channel;

GstStateChangeReturn ret;

static void on_negotiation_needed(GstElement *element, gpointer user_data);
static void send_ice_candidate_message(GstElement *webrtc G_GNUC_UNUSED, guint mlineindex,
                                       gchar *candidate, gpointer user_data G_GNUC_UNUSED);
static void on_ice_gathering_state_notify(GstElement *webrtcbin, GParamSpec *pspec,
                                          gpointer user_data);
static void on_incoming_stream(GstElement *webrtc, GstPad *pad, GstElement *pipe);
static void on_offer_created(GstPromise *promise, gpointer user_data);

static std::string post_offer(std::string offer_text);

int main(int argc, char *argv[])
{
    g_assert_cmphex(argc, ==, 2);
    GOptionContext *context;
    GError *error = nullptr;
    static GOptionEntry entries[] = {
        {NULL},
    };
    context = g_option_context_new("- gstreamer webrtc sendrecv demo");
    g_option_context_add_main_entries(context, entries, NULL);
    g_option_context_add_group(context, gst_init_get_option_group());
    if (!g_option_context_parse(context, &argc, &argv, &error))
    {
        gst_printerr("Error initializing: %s\n", error->message);
        return -1;
    }

    target_url = std::string{argv[1]};
    pipe1 =
        gst_parse_launch("webrtcbin bundle-policy=max-bundle name=" PIPE_LINE_NAME " " STUN_SERVER
                         "videotestsrc is-live=true pattern=ball ! videoconvert ! queue ! "
                         /* increase the default keyframe distance, browsers have really long
                          * periods between keyframes and rely on PLI events on packet loss to
                          * fix corrupted video.
                          */
                         "x264enc tune=zerolatency key-int-max=2000 ! "
                         /* picture-id-mode=15-bit seems to make TWCC stats behave better */
                         "rtph264pay config-interval=1 ! "
                         "queue ! " RTP_CAPS_H264 "96 ! " PIPE_LINE_NAME ". "
                         "audiotestsrc is-live=true wave=red-noise ! audioconvert ! audioresample ! queue ! opusenc ! rtpopuspay name=audiopay ! "
                         "queue ! " RTP_CAPS_OPUS "97 ! sendrecv. ",
                         &error);
    if (error)
    {
        gst_printerr("Failed to parse launch: %s\n", error->message);
        g_error_free(error);
        goto err;
    }

    webrtc1 = gst_bin_get_by_name(GST_BIN(pipe1), PIPE_LINE_NAME);
    g_assert_nonnull(webrtc1);

    g_signal_connect(webrtc1, "on-negotiation-needed",
                     G_CALLBACK(on_negotiation_needed), NULL);
    g_signal_connect(webrtc1, "on-ice-candidate",
                     G_CALLBACK(send_ice_candidate_message), NULL);
    g_signal_connect(webrtc1, "notify::ice-gathering-state",
                     G_CALLBACK(on_ice_gathering_state_notify), NULL);

    gst_element_set_state(pipe1, GST_STATE_READY);

    g_signal_connect(webrtc1, "pad-added", G_CALLBACK(on_incoming_stream),
                     pipe1);

    gst_object_unref(webrtc1);

    std::cout << ("👌Starting pipeline\n");

    ret = gst_element_set_state(GST_ELEMENT(pipe1), GST_STATE_PLAYING);

    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    if (loop)
        g_main_loop_unref(loop);

    if (pipe1)
    {
        gst_element_set_state(GST_ELEMENT(pipe1), GST_STATE_NULL);
        gst_print("Pipeline stopped\n");
        gst_object_unref(pipe1);
    }

    if (ret == GST_STATE_CHANGE_FAILURE)
        goto err;

    return 0;

err:
    if (loop)
        g_main_loop_unref(loop);
    if (pipe1)
        g_clear_object(&pipe1);
    if (webrtc1)
        webrtc1 = nullptr;
    return -1;
}

static void on_negotiation_needed(GstElement *element, gpointer user_data G_GNUC_UNUSED)
{
    std::cout << ("👌negotiation_needed") << std::endl;
    GstPromise *promise =
        gst_promise_new_with_change_func(on_offer_created, NULL, NULL);
    g_signal_emit_by_name(webrtc1, "create-offer", NULL, promise);
}

static void
send_ice_candidate_message(GstElement *webrtc G_GNUC_UNUSED, guint mlineindex,
                           gchar *candidate, gpointer user_data G_GNUC_UNUSED)
{
    std::cout << "👌should send ice candidae message" << std::endl;
    //   gchar *text;
    //   JsonObject *ice, *msg;

    //   if (app_state < PEER_CALL_NEGOTIATING) {
    //     cleanup_and_quit_loop ("Can't send ICE, not in call", APP_STATE_ERROR);
    //     return;
    //   }

    //   ice = json_object_new ();
    //   json_object_set_string_member (ice, "candidate", candidate);
    //   json_object_set_int_member (ice, "sdpMLineIndex", mlineindex);
    //   msg = json_object_new ();
    //   json_object_set_object_member (msg, "ice", ice);
    //   text = get_string_from_json_object (msg);
    //   json_object_unref (msg);

    //   soup_websocket_connection_send_text (ws_conn, text);
    //   g_free (text);
}

static void
on_ice_gathering_state_notify(GstElement *webrtcbin, GParamSpec *pspec,
                              gpointer user_data)
{
    GstWebRTCICEGatheringState ice_gather_state;
    const gchar *new_state = "unknown";

    g_object_get(webrtcbin, "ice-gathering-state", &ice_gather_state, NULL);
    switch (ice_gather_state)
    {
    case GST_WEBRTC_ICE_GATHERING_STATE_NEW:
        new_state = "new";
        break;
    case GST_WEBRTC_ICE_GATHERING_STATE_GATHERING:
        new_state = "gathering";
        break;
    case GST_WEBRTC_ICE_GATHERING_STATE_COMPLETE:
        new_state = "complete";
        break;
    }
    std::cout << "👌ICE gathering state changed to " << new_state << "\n";
}

static void on_incoming_stream(GstElement *webrtc, GstPad *pad, GstElement *pipe)
{
    std::cout << "👌on_incoming_stream\n";
    //   GstElement *decodebin;
    //   GstPad *sinkpad;

    //   if (GST_PAD_DIRECTION (pad) != GST_PAD_SRC)
    //     return;

    //   decodebin = gst_element_factory_make ("decodebin", NULL);
    //   g_signal_connect (decodebin, "pad-added",
    //       G_CALLBACK (on_incoming_decodebin_stream), pipe);
    //   gst_bin_add (GST_BIN (pipe), decodebin);
    //   gst_element_sync_state_with_parent (decodebin);

    //   sinkpad = gst_element_get_static_pad (decodebin, "sink");
    //   gst_pad_link (pad, sinkpad);
    //   gst_object_unref (sinkpad);
}

static void on_offer_created(GstPromise *promise, gpointer user_data)
{
    std::cout << ("👌offer created") << std::endl;
    GstWebRTCSessionDescription *offer = nullptr;
    const GstStructure *reply;

    //   g_assert_cmphex (app_state, ==, PEER_CALL_NEGOTIATING);

    g_assert_cmphex(gst_promise_wait(promise), ==, GST_PROMISE_RESULT_REPLIED);
    reply = gst_promise_get_reply(promise);
    gst_structure_get(reply, "offer",
                      GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &offer, NULL);
    gst_promise_unref(promise);

    promise = gst_promise_new();
    g_signal_emit_by_name(webrtc1, "set-local-description", offer, promise);
    // gst_promise_wait(promise);
    gst_promise_interrupt(promise);
    gst_promise_unref(promise);

    //   send_sdp_to_peer (offer);
    char *text = (char *)gst_sdp_message_as_text(offer->sdp);
    std::string offer_text{text};
    std::string answer_text{post_offer(offer_text)};

    GstSDPMessage *sdp = nullptr;
    GstWebRTCSessionDescription *answer = nullptr;

    gst_sdp_message_new(&sdp);
    GstSDPResult ret = gst_sdp_message_parse_buffer(
        (guint8 *)answer_text.c_str(),
        answer_text.length(),
        sdp);

    if (ret != GST_SDP_OK)
    {
        std::cout << ("❌Failed to parse SDP answer\n");
        gst_sdp_message_free(sdp);
        return;
    }

    // 3. 创建 WebRTC answer 描述
    answer = gst_webrtc_session_description_new(
        GST_WEBRTC_SDP_TYPE_ANSWER,
        sdp);

    // 4. 设置 remote description
    promise = gst_promise_new();
    g_signal_emit_by_name(
        webrtc1,
        "set-remote-description",
        answer,
        promise);

    gst_promise_interrupt(promise);
    gst_promise_unref(promise);

    // 5. 释放（注意：sdp 所有权已转移，不要单独 free）
    gst_webrtc_session_description_free(answer);
    gst_webrtc_session_description_free(offer);
    std::cout << ("✅ Remote description (answer) set\n");
}

static std::string post_offer(std::string offer_text)
{
    httplib::Client cli(target_url);
    cli.enable_server_certificate_verification(false);
    std::cout << ("👌onLocalDescription") << std::endl;
    httplib::Headers headers = {{"Authorization", "Bearer none"},
                                {"Content-Type", "application/sdp"}};
    auto res = cli.Post("/whip", headers, offer_text, "application/sdp");
    if (res && res->status / 200 == 1)
    {
        std::cout << "✅answer:\n"
                  << (res->body) << std::endl;
        return (res->body);
    }
    else
        throw("❌post_offer error");
}