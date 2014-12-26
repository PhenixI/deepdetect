/**
 * DeepDetect
 * Copyright (c) 2014 Emmanuel Benazera
 * Author: Emmanuel Benazera <beniz@droidnik.fr>
 *
 * This file is part of deepdetect.
 *
 * deepdetect is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * deepdetect is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with deepdetect.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "deepdetect.h"
#include "jsonapi.h"
#include <gtest/gtest.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

using namespace dd;

static std::string ok_str = "{\"status\":{\"code\":200,\"msg\":\"OK\"}}";
static std::string created_str = "{\"status\":{\"code\":201,\"msg\":\"Created\"}}";
static std::string bad_param_str = "{\"status\":{\"code\":400,\"msg\":\"BadRequest\"}}";
static std::string not_found_str = "{\"status\":{\"code\":404,\"msg\":\"NotFound\"}}";

static std::string mnist_repo = "../examples/caffe/mnist/";

TEST(caffeapi,service_train)
{
  ::google::InitGoogleLogging("ut_caffeapi");
  /*char current_path[FILENAME_MAX]; // FILENAME_MAX in stdio.h
  if (!getcwd(current_path, sizeof(current_path)))
    {
      ASSERT_EQ(0,errno);
    }
    std::cout << current_path << std::endl;*/

  // create service
  JsonAPI japi;
  std::string sname = "my_service";
  std::string jstr = "{\"mllib\":\"caffe\",\"description\":\"my classifier\",\"type\":\"supervised\",\"model\":{\"repository\":\"" +  mnist_repo + "\"},\"input\":\"image\"}";
  std::string joutstr = japi.service_create(sname,jstr);
  ASSERT_EQ(created_str,joutstr);

  // train
  std::string jtrainstr = "{\"service\":\"" + sname + "\",\"async\":false,\"parameters\":{\"mllib\":{\"solver\":{\"iterations\":100}}}}";
  joutstr = japi.service_train(jtrainstr);
  std::cout << "joutstr=" << joutstr << std::endl;
  JDoc jd;
  jd.Parse(joutstr.c_str());
  ASSERT_TRUE(!jd.HasParseError());
  ASSERT_TRUE(jd.HasMember("status"));
  ASSERT_EQ(201,jd["status"]["code"]);
  ASSERT_EQ("Created",jd["status"]["msg"]);
  ASSERT_TRUE(jd.HasMember("head"));
  ASSERT_EQ("/train",jd["head"]["method"]);
  ASSERT_TRUE(jd["head"]["time"].GetDouble() > 0);
  ASSERT_TRUE(jd.HasMember("body"));
  ASSERT_TRUE(jd["body"].HasMember("loss"));
  ASSERT_TRUE(jd["body"]["loss"].GetDouble() > 0);
}

TEST(caffeapi,service_train_async_status_delete)
{
  // create service
  JsonAPI japi;
  std::string sname = "my_service";
  std::string jstr = "{\"mllib\":\"caffe\",\"description\":\"my classifier\",\"type\":\"supervised\",\"model\":{\"repository\":\"" +  mnist_repo + "\"},\"input\":\"image\"}";
  std::string joutstr = japi.service_create(sname,jstr);
  ASSERT_EQ(created_str,joutstr);

  // train
  std::string jtrainstr = "{\"service\":\"" + sname + "\",\"async\":true,\"parameters\":{\"mllib\":{\"solver\":{\"iterations\":10000}}}}";
  joutstr = japi.service_train(jtrainstr);
  std::cout << "joutstr=" << joutstr << std::endl;
  JDoc jd;
  jd.Parse(joutstr.c_str());
  ASSERT_TRUE(!jd.HasParseError());
  ASSERT_TRUE(jd.HasMember("status"));
  ASSERT_EQ(201,jd["status"]["code"]);
  ASSERT_EQ("Created",jd["status"]["msg"]);
  ASSERT_TRUE(jd.HasMember("head"));
  ASSERT_EQ("/train",jd["head"]["method"]);
  ASSERT_EQ(1,jd["head"]["job"].GetInt());
  ASSERT_EQ("running",jd["head"]["status"]);
  
  // status.
  std::string jstatusstr = "{\"service\":\"" + sname + "\",\"job\":1,\"timeout\":5}";
  joutstr = japi.service_train_status(jstatusstr);
  std::cout << "joutstr=" << joutstr << std::endl;
  JDoc jd2;
  jd2.Parse(joutstr.c_str());
  ASSERT_TRUE(!jd2.HasParseError());
  ASSERT_TRUE(jd2.HasMember("status"));
  ASSERT_EQ(200,jd2["status"]["code"]);
  ASSERT_EQ("OK",jd2["status"]["msg"]);
  ASSERT_TRUE(jd2.HasMember("head"));
  ASSERT_EQ("/train",jd2["head"]["method"]);
  ASSERT_EQ(5.0,jd2["head"]["time"].GetDouble());
  ASSERT_EQ("running",jd2["head"]["status"]);
  ASSERT_EQ(1,jd2["head"]["job"]);
  ASSERT_TRUE(jd2.HasMember("body"));
  ASSERT_TRUE(jd2["body"].HasMember("loss"));
  ASSERT_TRUE(jd2["body"]["loss"].GetDouble() > 0); // may fail on a slow machine

  // delete job.
  std::string jdelstr = "{\"service\":\"" + sname + "\",\"job\":1}";
  joutstr = japi.service_train_delete(jdelstr);
  std::cout << "joutstr=" << joutstr << std::endl;
  JDoc jd3;
  jd3.Parse(joutstr.c_str());
  ASSERT_TRUE(!jd3.HasParseError());
  ASSERT_TRUE(jd3.HasMember("status"));
  ASSERT_EQ(200,jd3["status"]["code"]);
  ASSERT_EQ("OK",jd3["status"]["msg"]);
  ASSERT_TRUE(jd3.HasMember("head"));
  ASSERT_EQ("/train",jd3["head"]["method"]);
  ASSERT_TRUE(jd3["head"]["time"].GetDouble() > 0);
  ASSERT_EQ("terminated",jd3["head"]["status"]);
  ASSERT_EQ(1,jd3["head"]["job"].GetInt());
}

TEST(caffeapi,service_train_async_final_status)
{
  // create service
  JsonAPI japi;
  std::string sname = "my_service";
  std::string jstr = "{\"mllib\":\"caffe\",\"description\":\"my classifier\",\"type\":\"supervised\",\"model\":{\"repository\":\"" +  mnist_repo + "\"},\"input\":\"image\"}";
  std::string joutstr = japi.service_create(sname,jstr);
  ASSERT_EQ(created_str,joutstr);

  // train
  std::string jtrainstr = "{\"service\":\"" + sname + "\",\"async\":true,\"parameters\":{\"mllib\":{\"solver\":{\"iterations\":150}}}}";
  joutstr = japi.service_train(jtrainstr);
  std::cout << "joutstr=" << joutstr << std::endl;
  JDoc jd;
  jd.Parse(joutstr.c_str());
  ASSERT_TRUE(!jd.HasParseError());
  ASSERT_TRUE(jd.HasMember("status"));
  ASSERT_EQ(201,jd["status"]["code"]);
  ASSERT_EQ("Created",jd["status"]["msg"]);
  ASSERT_TRUE(jd.HasMember("head"));
  ASSERT_EQ("/train",jd["head"]["method"]);
  ASSERT_EQ(1,jd["head"]["job"].GetInt());
  ASSERT_EQ("running",jd["head"]["status"]);
  
  // status.
  bool running = true;
  while(running)
    {
      //sleep(1);
      std::string jstatusstr = "{\"service\":\"" + sname + "\",\"job\":1,\"timeout\":1}";
      joutstr = japi.service_train_status(jstatusstr);
      std::cout << "joutstr=" << joutstr << std::endl;
      running = joutstr.find("running") != std::string::npos;
      if (!running)
	{
	  JDoc jd2;
	  jd2.Parse(joutstr.c_str());
	  ASSERT_TRUE(!jd2.HasParseError());
	  ASSERT_TRUE(jd2.HasMember("status"));
	  ASSERT_EQ(200,jd2["status"]["code"]);
	  ASSERT_EQ("OK",jd2["status"]["msg"]);
	  ASSERT_TRUE(jd2.HasMember("head"));
	  ASSERT_EQ("/train",jd2["head"]["method"]);
	  ASSERT_TRUE(jd2["head"]["time"].GetDouble() > 0);
	  ASSERT_EQ("finished",jd2["head"]["status"]);
	  ASSERT_EQ(1,jd2["head"]["job"]);
	  ASSERT_TRUE(jd2.HasMember("body"));
	  ASSERT_TRUE(jd2["body"].HasMember("loss"));
	  ASSERT_TRUE(jd2["body"]["loss"].GetDouble() > 0);
	}
    }
}
