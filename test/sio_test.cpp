//
//  sio_test_sample.cpp
//
//  Created by Melo Yao on 3/24/15.
//

#include <stdio.h>
#include <sio_client.h>
#include <internal/sio_packet.h>
#include <functional>
#include <iostream>
#include <thread>

#include "gtest/gtest.h"

#ifndef _WIN32
#include "json.hpp" //nlohmann::json cannot build in MSVC
#endif


using namespace sio;


TEST( testPacket, construct1 )
{
    packet p("/nsp",nullptr,1001,true);
    EXPECT_EQ(p.get_frame(), packet::frame_message);
    EXPECT_EQ(p.get_message(), nullptr);
    EXPECT_EQ(p.get_nsp(), std::string("/nsp"));
    EXPECT_EQ(p.get_pack_id(), 1001);
}

TEST( testPacket, construct2 )
{
    packet p(packet::frame_ping);
    EXPECT_EQ(p.get_frame(), packet::frame_ping);
    EXPECT_EQ(p.get_message(), nullptr);
    EXPECT_EQ(p.get_nsp(), std::string(""));
    EXPECT_EQ(p.get_pack_id(), 0xFFFFFFFF);
}

TEST( testPacket, construct3 )
{
    packet p(packet::type_connect,"/nsp",nullptr);
    EXPECT_EQ(p.get_frame(), packet::frame_message);
    EXPECT_EQ(p.get_type(), packet::type_connect);
    EXPECT_EQ(p.get_message(), nullptr);
    EXPECT_EQ(p.get_nsp(), std::string("/nsp"));
    EXPECT_EQ(p.get_pack_id(), 0xFFFFFFFF);
}

TEST( testPacket, accept1 )
{
    packet p(packet::type_connect,"/nsp",nullptr);
    std::string payload;
    std::vector<std::shared_ptr<const std::string> > buffers;
    p.accept(payload,buffers);
    EXPECT_EQ(buffers.size(), 0);
    EXPECT_EQ(payload, "40/nsp") << std::string("outputing payload:") + payload;
}

TEST( testPacket, accept2 )
{
    packet p(packet::frame_ping);
    std::string payload;
    std::vector<std::shared_ptr<const std::string> > buffers;
    p.accept(payload,buffers);
    EXPECT_EQ(buffers.size(), 0);
    EXPECT_EQ(payload, "2") << std::string("outputing payload:") + payload;
}

TEST( testPacket, accept3 )
{
    message::ptr array = array_message::create();
    array->get_vector().push_back(string_message::create("event"));
    array->get_vector().push_back(string_message::create("text"));
    packet p("/nsp",array,1001,true);
    std::string payload;
    std::vector<std::shared_ptr<const std::string> > buffers;
    p.accept(payload,buffers);
    EXPECT_EQ(p.get_type(), packet::type_ack);
    EXPECT_EQ(buffers.size(), 0);
    EXPECT_EQ(payload, "43/nsp,1001[\"event\",\"text\"]") << std::string("outputing payload:")+payload;
}

#ifndef _WIN32
TEST( testPacket, accept4 )
{
    message::ptr binObj = object_message::create();
    binObj->get_map()["desc"] = string_message::create("Bin of 100 bytes");
    char bin[100];
    memset(bin,0,100*sizeof(char));
    binObj->get_map()["bin1"] = binary_message::create(std::shared_ptr<const std::string>(new std::string(bin,100)));
    char bin2[50];
    memset(bin2,1,50*sizeof(char));
    binObj->get_map()["bin2"] = binary_message::create(std::make_shared<const std::string>(bin2,50));

    packet p("/nsp",binObj,1001,false);
    std::string payload;
    std::vector<std::shared_ptr<const std::string> > buffers;
    p.accept(payload,buffers);
    EXPECT_EQ(p.get_type(), packet::type_binary_event);
    EXPECT_EQ(buffers.size(), 2);
    size_t json_start = payload.find("{");
    EXPECT_NE(json_start, std::string::npos);
    std::string header = payload.substr(0,json_start);
    EXPECT_EQ(header, "452-/nsp,1001") << std::string("outputing payload header:") + header;
    std::string json = payload.substr(json_start);
    nlohmann::json j = nlohmann::json::parse(json);
    EXPECT_EQ(j["desc"].get<std::string>(), "Bin of 100 bytes") << std::string("outputing payload desc:") + j["desc"].get<std::string>();
    EXPECT_TRUE((bool)j["bin1"]["_placeholder"]) << std::string("outputing payload bin1:") + j["bin1"].dump();
    EXPECT_TRUE((bool)j["bin2"]["_placeholder"]) << std::string("outputing payload bin2:") + j["bin2"].dump();
    int bin1Num = j["bin1"]["num"].get<int>();
    char numchar[] = {0,0};
    numchar[0] = bin1Num+'0';
    EXPECT_EQ(buffers[bin1Num]->length(), 101) << std::string("outputing payload bin1 num:") + numchar;
    EXPECT_EQ(buffers[bin1Num]->at(50), 0);
    EXPECT_EQ(buffers[bin1Num]->at(0), packet::frame_message);
    int bin2Num = j["bin2"]["num"].get<int>();
    numchar[0] = bin2Num+'0';
    EXPECT_EQ(buffers[bin2Num]->length(), 51) << std::string("outputing payload bin2 num:") + numchar;
    EXPECT_EQ(buffers[bin2Num]->at(25), 1);
    EXPECT_EQ(buffers[bin2Num]->at(0), packet::frame_message);
}
#endif

TEST( testPacket, parse1 )
{
    packet p;
    bool hasbin = p.parse("42/nsp,1001[\"event\",\"text\"]");
    EXPECT_FALSE(hasbin);
    EXPECT_EQ(p.get_frame(), packet::frame_message);
    EXPECT_EQ(p.get_type(), packet::type_event);
    EXPECT_EQ(p.get_nsp(), "/nsp");
    EXPECT_EQ(p.get_pack_id(), 1001);
    EXPECT_EQ(p.get_message()->get_flag(), message::flag_array);

    ASSERT_EQ(p.get_message()->get_vector()[0]->get_flag(), message::flag_string);
    EXPECT_EQ(p.get_message()->get_vector()[0]->get_string(), "event");

    ASSERT_EQ(p.get_message()->get_vector()[1]->get_flag(), message::flag_string);
    EXPECT_EQ(p.get_message()->get_vector()[1]->get_string(), "text");

    hasbin = p.parse("431111[\"ack\",{\"count\":5}]");
    EXPECT_FALSE(hasbin);
    EXPECT_EQ(p.get_frame(), packet::frame_message);
    EXPECT_EQ(p.get_type(), packet::type_ack);
    EXPECT_EQ(p.get_pack_id(), 1111);
    EXPECT_EQ(p.get_nsp(), "/");
    EXPECT_EQ(p.get_message()->get_flag(), message::flag_array);

    ASSERT_EQ(p.get_message()->get_vector()[0]->get_flag(), message::flag_string);
    EXPECT_EQ(p.get_message()->get_vector()[0]->get_string(), "ack");

    ASSERT_EQ(p.get_message()->get_vector()[1]->get_flag(), message::flag_object);
    EXPECT_EQ(p.get_message()->get_vector()[1]->get_map()["count"]->get_int(), 5);
}

TEST( testPacket, parse2 )
{
    packet p;
    bool hasbin = p.parse("3");
    EXPECT_FALSE(hasbin);
    EXPECT_EQ(p.get_frame(), packet::frame_pong);
    EXPECT_FALSE(p.get_message());
    EXPECT_EQ(p.get_nsp(), "/");
    EXPECT_EQ(p.get_pack_id(), -1);
    hasbin = p.parse("2");

    EXPECT_FALSE(hasbin);
    EXPECT_EQ(p.get_frame(), packet::frame_ping);
    EXPECT_FALSE(p.get_message());
    EXPECT_EQ(p.get_nsp(), "/");
    EXPECT_EQ(p.get_pack_id(), -1);
}

TEST( testPacket, parse3 )
{
    packet p;
    bool hasbin = p.parse("40/nsp");
    EXPECT_FALSE(hasbin);
    EXPECT_EQ(p.get_type(), packet::type_connect);
    EXPECT_EQ(p.get_frame(), packet::frame_message);
    EXPECT_EQ(p.get_nsp(), "/nsp");
    EXPECT_EQ(p.get_pack_id(), -1);
    EXPECT_FALSE(p.get_message());
    p.parse("40");
    EXPECT_EQ(p.get_type(), packet::type_connect);
    EXPECT_EQ(p.get_nsp(), "/");
    EXPECT_EQ(p.get_pack_id(), -1);
    EXPECT_FALSE(p.get_message());
    p.parse("44\"error\"");
    EXPECT_EQ(p.get_type(), packet::type_error);
    EXPECT_EQ(p.get_nsp(), "/");
    EXPECT_EQ(p.get_pack_id(), -1);
    EXPECT_EQ(p.get_message()->get_flag(), message::flag_string);
    p.parse("44/nsp,\"error\"");
    EXPECT_EQ(p.get_type(), packet::type_error);
    EXPECT_EQ(p.get_nsp(), "/nsp");
    EXPECT_EQ(p.get_pack_id(), -1);
    EXPECT_EQ(p.get_message()->get_flag(), message::flag_string);
}

TEST( testPacket, parse4 )
{
    packet p;
    bool hasbin = p.parse("452-/nsp,101[\"bin_event\",[{\"_placeholder\":true,\"num\":1},{\"_placeholder\":true,\"num\":0},\"text\"]]");
    EXPECT_TRUE(hasbin);
    char buf[101];
    buf[0] = packet::frame_message;
    memset(buf+1,0,100);

    std::string bufstr(buf,101);
    std::string bufstr2(buf,51);
    EXPECT_TRUE(p.parse_buffer(bufstr));
    EXPECT_FALSE(p.parse_buffer(bufstr2));

    EXPECT_EQ(p.get_frame(), packet::frame_message);
    EXPECT_EQ(p.get_nsp(), "/nsp");
    EXPECT_EQ(p.get_pack_id(), 101);
    message::ptr msg = p.get_message();
    ASSERT_TRUE(msg);
    EXPECT_EQ(msg->get_flag(), message::flag_array);
    EXPECT_EQ(msg->get_vector()[0]->get_string(), "bin_event");
    message::ptr array = msg->get_vector()[1];
    ASSERT_EQ(array->get_flag(), message::flag_array);
    ASSERT_EQ(array->get_vector()[0]->get_flag(), message::flag_binary);
    ASSERT_EQ(array->get_vector()[1]->get_flag(), message::flag_binary);
    ASSERT_EQ(array->get_vector()[2]->get_flag(), message::flag_string);
    EXPECT_EQ(array->get_vector()[0]->get_binary()->size(), 50);
    EXPECT_EQ(array->get_vector()[1]->get_binary()->size(), 100);
    EXPECT_EQ(array->get_vector()[2]->get_string(), "text");

}

int main(int argc, char **argv) {
  printf("Running main() from gtest_main.cc\n");
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
