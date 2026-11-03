#include "SimGearSPM.h"
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/bucket/newbucket.hxx>
#include <simgear/scene/util/SGImageUtils.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/io/DNSClient.hxx>
#include <simgear/sound/soundmgr.hxx>
#include <osgDB/Registry>
#include <sstream>
#include <string>
#include <cstdio>
#include <simgear/io/raw_socket.hxx>
#include <chrono>
#include <thread>
int main() {
    SimGearSPMInitialize();
    for (const char* extension : {"ac", "btg", "stg", "gltf"})
        if (!osgDB::Registry::instance()->getReaderWriterForExtension(extension)) return 6;
    if (!SimGearSPMVersion()[0]) return 1;
    SGPropertyNode root;
    std::istringstream xml("<PropertyList><controls><flight><elevator type=\"double\">-0.25</elevator></flight></controls><aircraft>C172P</aircraft></PropertyList>");
    readProperties(xml,&root);
    if(root.getDoubleValue("controls/flight/elevator") != -0.25) return 2;
    if(std::string(root.getStringValue("aircraft")) != "C172P") return 3;
    SGBucket bucket(-0.3,50.8);
    if (!bucket.isValid() || SGBucket(bucket.gen_index()).gen_index() != bucket.gen_index()) return 4;
    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->allocateImage(2,2,1,GL_RGBA,GL_UNSIGNED_BYTE);
    simgear::ImageUtils::markAsUnNormalized(image.get(),true);
    if (!simgear::ImageUtils::isUnNormalized(image.get())) return 5;
    simgear::HTTP::Client http;
    simgear::DNS::Client dns;
    SGSoundMgr sound;
    simgear::IPAddress local;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!simgear::IPAddress::lookupNonblocking("localhost", local)) {
        if (std::chrono::steady_clock::now() > deadline) return 7;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!local.isValid()) return 8;
    // Leave requests pending: atexit must stop/join even if a notification
    // arrives while the resolver has temporarily released its state mutex.
    for (int i=2; i<32; ++i) {
        simgear::IPAddress pending;
        const auto host = "127.0.0." + std::to_string(i);
        simgear::IPAddress::lookupNonblocking(host.c_str(), pending);
    }
    std::puts("PASS resolver delivery and cooperative exit");
    std::puts("PASS SimGear property XML/elevator, scenery bucket, image metadata, HTTP/DNS and sound construction");
    return 0;
}
