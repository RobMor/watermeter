#include <cstdlib> // Exit function
#include <iostream> // Printing and stuff

#include "webcam.h"

gboolean BusHandler(GstBus *bus, GstMessage *msg, gpointer *data);

WebCam::WebCam() {
    Gst::init();

    /* ------ Create pipeline ------ */
    pipeline_ = Gst::Pipeline::create();

    /* ------ Create pipeline elements ------ */

    // Video4Linux source (the driver for the webcam)
    Glib::RefPtr<Gst::Element> source = Gst::ElementFactory::create_element("v4l2src");

    // A converter to convert the data from the webcam to any format we could
    // want (in this case we want raw rgb video)
    Glib::RefPtr<Gst::VideoConvert> converter = Gst::VideoConvert::create();

    // GstAppSink that we can get data from
    sink_ = Gst::AppSink::create();
    // Only allow the AppSink to only store one buffer (aka frame)
    sink_->property_max_buffers().set_value(1);
    // Tell the AppSink to drop old buffers when we run out of space
    sink_->property_drop().set_value(true);
    // Tell the AppSink to let us know when there is a new frame available
    sink_->property_emit_signals().set_value(true);
    // We only want raw RGB data in this sink so we make caps to filter out
    // everything else
    Glib::RefPtr<Gst::Caps> caps = Gst::Caps::create_simple("video/x-raw");
    caps->set_simple("format", "RGB"); // TODO this might be wrong
    // This is what it used to be
    // caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB", NULL);
    // Apply the caps to our sink
    sink_->property_caps().set_value(caps);

    /* ------ Set up pipeline ------ */

    // Add a message handler to the pipeline (called bus handler in GST world)
    Glib::RefPtr<Gst::Bus> bus = pipeline_->get_bus();
    bus->add_watch(sigc::mem_fun(this, &WebCam::BusHandler));
    
    // Add all previously created elements to the pipeline
    pipeline_->add(source)->add(converter)->add(sink_);

    // Link the elements together
    source->link(converter)->link(sink_);
}

WebCam::~WebCam() {
    // Stop the pipeline
    pipeline_->set_state(Gst::State::STATE_NULL);
}

void WebCam::Init() {
    // Start the pipeline and block until it actually starts
    Gst::StateChangeReturn change = pipeline_->set_state(Gst::State::STATE_PLAYING);

    if (change == Gst::StateChangeReturn::STATE_CHANGE_ASYNC) {
        Gst::State state, pending;
        Gst::StateChangeReturn ret = pipeline_->get_state(state, pending, Gst::CLOCK_TIME_NONE);
        
        if (ret == Gst::StateChangeReturn::STATE_CHANGE_FAILURE) {
            std::cerr << "Failed to wait for pipeline to start playing" << std::endl;
            exit(EXIT_FAILURE);
        }
    } else if (change == Gst::StateChangeReturn::STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to set pipeline to playing" << std::endl;
        exit(EXIT_FAILURE);
    }
}

Glib::SignalProxy<Gst::FlowReturn> WebCam::signal_new_frame() {
    return sink_->signal_new_sample();
}

Glib::RefPtr<Gdk::Pixbuf> WebCam::Capture() {
    // Pull a sample (aka frame) from the sink blocking until one is available
    Glib::RefPtr<Gst::Sample> sample = sink_->pull_sample();

    if (sample) {
        // Get the caps from the sample
        Glib::RefPtr<Gst::Caps> caps = sample->get_caps();

        if (!caps) {
            std::cerr << "Could not get format of frame" << std::endl;
            exit(EXIT_FAILURE);
        }

        // Get the structure behind the caps so we can grab height/width from
        // it
        Gst::Structure structure = caps->get_structure(0);
        
        bool success = false;
        int width = 0, height = 0;
        success |= structure.get_field("width", width);
        success |= structure.get_field("height", height);

        if (!success) {
            std::cerr << "Could not get dimensions of frame" << std::endl;
            exit(EXIT_FAILURE);
        }

        // Get the buffer from the sample so we can access the raw data
        Glib::RefPtr<Gst::Buffer> buffer = sample->get_buffer();

        if (!buffer) {
            std::cerr << "Could not retreive buffer" << std::endl;
            exit(EXIT_FAILURE);
        }

        Gst::MapInfo map_info;
        // Get the spot in memory that the data is at.
        if (buffer->map(map_info, Gst::MapFlags::MAP_READ)) {
            Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_data(
                map_info.get_data(), Gdk::Colorspace::COLORSPACE_RGB, FALSE, 8,
                width, height, GST_ROUND_UP_4(width * 3));

            buffer->unmap(map_info);

            return pixbuf;
        } else {
            std::cerr << "Could not retreive buffer data" << std::endl;
            exit(EXIT_FAILURE);
        }
    } else {
        if (sink_->property_eos().get_value()) {
            std::cerr << "Could not get frame because there are no more frames left" << std::endl;
        } else {
            std::cerr << "Could not get frame (not EOS)" << std::endl;
        }

        exit(EXIT_FAILURE);
    }
}

// A callback for the GStreamer bus. Basically handles any errors that occur.
bool WebCam::BusHandler(const Glib::RefPtr<Gst::Bus> &, const Glib::RefPtr<Gst::Message> &message) {
    switch (message->get_message_type()) {
    case Gst::MessageType::MESSAGE_EOS:
        std::cerr << "End of stream" << std::endl;
        exit(EXIT_FAILURE);
        break;
    case Gst::MessageType::MESSAGE_ERROR:
        std::cerr << "Error: " << Glib::RefPtr<Gst::MessageError>::cast_static(message)->parse_debug() << std::endl;
        exit(EXIT_FAILURE);
        break;
    default:
        return TRUE;
    }
}
