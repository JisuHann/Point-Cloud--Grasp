// Generated by gencpp from file task_assembly/door_open_planner.msg
// DO NOT EDIT!


#ifndef TASK_ASSEMBLY_MESSAGE_DOOR_OPEN_PLANNER_H
#define TASK_ASSEMBLY_MESSAGE_DOOR_OPEN_PLANNER_H

#include <ros/service_traits.h>


#include <task_assembly/door_open_plannerRequest.h>
#include <task_assembly/door_open_plannerResponse.h>


namespace task_assembly
{

struct door_open_planner
{

typedef door_open_plannerRequest Request;
typedef door_open_plannerResponse Response;
Request request;
Response response;

typedef Request RequestType;
typedef Response ResponseType;

}; // struct door_open_planner
} // namespace task_assembly


namespace ros
{
namespace service_traits
{


template<>
struct MD5Sum< ::task_assembly::door_open_planner > {
  static const char* value()
  {
    return "b102701686e4ba528d2b5ce877e202a8";
  }

  static const char* value(const ::task_assembly::door_open_planner&) { return value(); }
};

template<>
struct DataType< ::task_assembly::door_open_planner > {
  static const char* value()
  {
    return "task_assembly/door_open_planner";
  }

  static const char* value(const ::task_assembly::door_open_planner&) { return value(); }
};


// service_traits::MD5Sum< ::task_assembly::door_open_plannerRequest> should match
// service_traits::MD5Sum< ::task_assembly::door_open_planner >
template<>
struct MD5Sum< ::task_assembly::door_open_plannerRequest>
{
  static const char* value()
  {
    return MD5Sum< ::task_assembly::door_open_planner >::value();
  }
  static const char* value(const ::task_assembly::door_open_plannerRequest&)
  {
    return value();
  }
};

// service_traits::DataType< ::task_assembly::door_open_plannerRequest> should match
// service_traits::DataType< ::task_assembly::door_open_planner >
template<>
struct DataType< ::task_assembly::door_open_plannerRequest>
{
  static const char* value()
  {
    return DataType< ::task_assembly::door_open_planner >::value();
  }
  static const char* value(const ::task_assembly::door_open_plannerRequest&)
  {
    return value();
  }
};

// service_traits::MD5Sum< ::task_assembly::door_open_plannerResponse> should match
// service_traits::MD5Sum< ::task_assembly::door_open_planner >
template<>
struct MD5Sum< ::task_assembly::door_open_plannerResponse>
{
  static const char* value()
  {
    return MD5Sum< ::task_assembly::door_open_planner >::value();
  }
  static const char* value(const ::task_assembly::door_open_plannerResponse&)
  {
    return value();
  }
};

// service_traits::DataType< ::task_assembly::door_open_plannerResponse> should match
// service_traits::DataType< ::task_assembly::door_open_planner >
template<>
struct DataType< ::task_assembly::door_open_plannerResponse>
{
  static const char* value()
  {
    return DataType< ::task_assembly::door_open_planner >::value();
  }
  static const char* value(const ::task_assembly::door_open_plannerResponse&)
  {
    return value();
  }
};

} // namespace service_traits
} // namespace ros

#endif // TASK_ASSEMBLY_MESSAGE_DOOR_OPEN_PLANNER_H