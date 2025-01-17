/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: voxel_grid.cpp 35876 2011-02-09 01:04:36Z rusu $
 *
 */

#include <pluginlib/class_list_macros.hpp>
#include "pcl_ros/filters/voxel_grid.h"

//////////////////////////////////////////////////////////////////////////////////////////////
bool
pcl_ros::VoxelGrid::child_init (ros::NodeHandle &nh, bool &has_service)
{
  // Enable the dynamic reconfigure service
  has_service = true;
  srv_ = boost::make_shared <dynamic_reconfigure::Server<pcl_ros::VoxelGridConfig> > (nh);
  dynamic_reconfigure::Server<pcl_ros::VoxelGridConfig>::CallbackType f = boost::bind (&VoxelGrid::config_callback, this, boost::placeholders::_1, boost::placeholders::_2);
  srv_->setCallback (f);

  return (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////
void
pcl_ros::VoxelGrid::filter (const PointCloud2::ConstPtr &input, 
                            const IndicesPtr &indices, 
                            PointCloud2 &output)
{
  boost::mutex::scoped_lock lock (mutex_);
  pcl::PCLPointCloud2::Ptr pcl_input(new pcl::PCLPointCloud2);
  pcl_conversions::toPCL (*(input), *(pcl_input));
  impl_.setInputCloud (pcl_input);
  impl_.setIndices (indices);
  pcl::PCLPointCloud2 pcl_output;
  impl_.filter (pcl_output);
  pcl_conversions::moveFromPCL(pcl_output, output);
}

//////////////////////////////////////////////////////////////////////////////////////////////
void
pcl_ros::VoxelGrid::config_callback (pcl_ros::VoxelGridConfig &config, uint32_t /*level*/)
{
  boost::mutex::scoped_lock lock (mutex_);

  Eigen::Vector3f leaf_size = impl_.getLeafSize ();
  Eigen::Vector3f config_leaf_size;

  config_leaf_size[0] = (config.leaf_size_x >= 0.0 ? config.leaf_size_x : config.leaf_size);
  config_leaf_size[1] = (config.leaf_size_y >= 0.0 ? config.leaf_size_y : config.leaf_size);
  config_leaf_size[2] = (config.leaf_size_z >= 0.0 ? config.leaf_size_z : config.leaf_size);

  if (leaf_size != config_leaf_size) {
    NODELET_DEBUG("[config_callback] Setting the downsampling leaf size to: (%f, %f, %f).",
      config_leaf_size[0], config_leaf_size[1], config_leaf_size[2]);
    impl_.setLeafSize(config_leaf_size[0], config_leaf_size[1], config_leaf_size[2]);
  }
  unsigned int minPointsPerVoxel = impl_.getMinimumPointsNumberPerVoxel ();
  if (minPointsPerVoxel != ((unsigned int) config.min_points_per_voxel))
  {
    minPointsPerVoxel = config.min_points_per_voxel;
    NODELET_DEBUG ("[config_callback] Setting the minimum points per voxel to: %u.", minPointsPerVoxel);
    impl_.setMinimumPointsNumberPerVoxel (minPointsPerVoxel);
  }

  double filter_min, filter_max;
  impl_.getFilterLimits (filter_min, filter_max);
  if (filter_min != config.filter_limit_min)
  {
    filter_min = config.filter_limit_min;
    NODELET_DEBUG ("[config_callback] Setting the minimum filtering value a point will be considered from to: %f.", filter_min);
  }
  if (filter_max != config.filter_limit_max)
  {
    filter_max = config.filter_limit_max;
    NODELET_DEBUG ("[config_callback] Setting the maximum filtering value a point will be considered from to: %f.", filter_max);
  }
  impl_.setFilterLimits (filter_min, filter_max);

  if (impl_.getFilterLimitsNegative () != config.filter_limit_negative)
  {
    impl_.setFilterLimitsNegative (config.filter_limit_negative);
    NODELET_DEBUG ("[%s::config_callback] Setting the filter negative flag to: %s.", getName ().c_str (), config.filter_limit_negative ? "true" : "false");
  }

  if (impl_.getFilterFieldName () != config.filter_field_name)
  {
    impl_.setFilterFieldName (config.filter_field_name);
    NODELET_DEBUG ("[config_callback] Setting the filter field name to: %s.", config.filter_field_name.c_str ());
  }

  // ---[ These really shouldn't be here, and as soon as dynamic_reconfigure improves, we'll remove them and inherit from Filter
  if (tf_input_frame_ != config.input_frame)
  {
    tf_input_frame_ = config.input_frame;
    NODELET_DEBUG ("[config_callback] Setting the input TF frame to: %s.", tf_input_frame_.c_str ());
  }
  if (tf_output_frame_ != config.output_frame)
  {
    tf_output_frame_ = config.output_frame;
    NODELET_DEBUG ("[config_callback] Setting the output TF frame to: %s.", tf_output_frame_.c_str ());
  }
  // ]---
}

typedef pcl_ros::VoxelGrid VoxelGrid;
PLUGINLIB_EXPORT_CLASS(VoxelGrid,nodelet::Nodelet)

