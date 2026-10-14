// Central radio station list
// Edit this file to add, remove, or modify stations

struct RadioStation {
  const char* name;
  const char* url;
};

static const RadioStation STATIONS[] = {
  { "Qmusic",    "https://stream.qmusic.nl/qmusic/mp3" },
  { "Radio 538", "https://playerservices.streamtheworld.com/api/livestream-redirect/RADIO538.mp3" },
  { "Sky Radio", "https://playerservices.streamtheworld.com/api/livestream-redirect/SKYRADIO.mp3" },
  { "Veronica",  "https://playerservices.streamtheworld.com/api/livestream-redirect/VERONICA.mp3" },

/*  { "Qmusic",    "https://stream.qmusic.nl/qmusic/mp3" },
  { "Radio 538", "https://www.mp3streams.nl/zender/radio-538/stream/4-mp3-128" },
  { "Sky Radio", "https://www.mp3streams.nl/zender/skyradio/stream/8-mp3-128" },
  { "Veronica",  "https://www.mp3streams.nl/zender/veronica/stream/11-mp3-128" },
*/
};

static const int NUM_STATIONS = sizeof(STATIONS) / sizeof(STATIONS[0]);
