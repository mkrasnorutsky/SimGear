#include "SimGearSPM.h"
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/bucket/newbucket.hxx>
#include <simgear/scene/util/SGImageUtils.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/io/DNSClient.hxx>
#include <simgear/sound/soundmgr.hxx>
#include <sstream>
#include <string>
#include <cstdio>
int main() {
    SimGearSPMInitialize();
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
    std::puts("PASS SimGear property XML/elevator, scenery bucket, image metadata, HTTP/DNS and sound construction");
    return 0;
}
