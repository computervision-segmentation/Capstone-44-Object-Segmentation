/******************************************************************************
* Author: Sean Hendrickson
* File: demo_02_functionTets.cpp
* Last Modified: 24 February 2016
* Description: This file filters a PCD file to get only components in a
*              floor plane.  Assumptions are that only one object is
*              on the floor plane and that the lowest plane is the floor.
*              This program uses PCL for depth vision cameras.
******************************************************************************/
#include <pcl/common/common.h>
#include <pcl/filters/passthrough.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/statistical_outlier_removal.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/io/pcd_io.h>
#include <pcl/ModelCoefficients.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/sample_consensus/method_types.h>
#include <pcl/sample_consensus/model_types.h>
#include <pcl/segmentation/extract_polygonal_prism_data.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/surface/convex_hull.h>
#include <pcl/visualization/cloud_viewer.h>

#include <iostream>
#include <iomanip>
#include <stdlib.h>

#define LINUX
#define DEBUG
#define Z_CUTOFF 2.5     // 1 meter from camera

namespace c44
{
/** 
 * desc:  This function cuts off all points outside a given range
 *        on the field passed the the field parameter.
 * param: (in) sourceCloud - ptr to point cloud 
 *        (out) filteredCloud - ptr to cloud after filtering
 *        (in) axis - axis to filter values passed as "x", "y", or "z"
 *        (in) minPoint - kept points are >= minPoint
 *        (in) maxPoint - kept points are <= maxPoint
 */
void passthroughFilter(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,
	                   pcl::PointCloud<pcl::PointXYZ>::Ptr filteredCloud,
	                   const std::string& axis, float minPoint, float maxPoint);
					   
/**
 * desc: This function uses a voxel filter to downsample a PCD file.
 * param: (in) sourceCloud - ptr to cloud to be filtered
 *        (out) filteredCloud - ptr to cloud after filtering
 *        (in) leafSize - size of voxel in cm^3. (ex. 0.01 = 1 cm)
 */					   
void voxelFilter(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,
                 pcl::PointCloud<pcl::PointXYZ>::Ptr filteredCloud,
                 float leafSize);
				 
/**
 * desc: This function computes the average distance between each point in a PCD
 *       file and then uses the standard deviation to designate points as outliers.
 *       All outliers are then removed from the resultant point cloud.
 * param: (in) sourceCloud - ptr to input cloud
 *        (out) filteredCloud - ptr to cloud with outliers removed
 *        (in) neighborsToAnalyze - number of nearest neighbors to analyze (ex. 50)
 *        (in) stdDeviation - standard deviation used to find outliers (ex. 1.0)   
 */
void removeNoise(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,
                 pcl::PointCloud<pcl::PointXYZ>::Ptr filteredCloud,
                 int neighborsToAnalyze, double stdDeviation);
				 
/** 
 * desc: This function takes a source point cloud and segments out a plane.
 *       The inliers of the plane, as well as all information needed to
 *       rebuild the plane is returned through the function parameters.
 * param: (in) sourceCloud - ptr to input cloud
 *        (out) inliersCloud - returns inliers of plane as pointcloud 
 *        (out) outliersCloud - returns outliers of plane as pointcloud
 *        (out) coefficients - returns the coefficients of the plane
 *        (out) indices - returns the indices of the plane
 *        (in) minRadius - minimum radius between plane and camera in radians (ex. 0)
 *        (in) maxRadius - maximum radius between plane and camera in radians (ex. 1.57)
 *        (in) ransacIterations - maximum amount of times RANSAC can estimate plane (ex. 1000)
 *        (in) thresholdDistance - range of points considered inliers from plane model (ex 0.01 = 1cm)
 * pre-cond: all output parameters must be declared before function call
 * ret: -1 if a plane could not be found else
 *      -2 if source PCD is empty
 *       0 if succesfull
 */				 
int getPlane(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,  
             pcl::PointCloud<pcl::PointXYZ>::Ptr planeCloud,                 
             pcl::PointCloud<pcl::PointXYZ>::Ptr outliersCloud,              
             pcl::ModelCoefficients::Ptr coefficients,                       
             pcl::PointIndices::Ptr indices,	
             double minRadius, double maxRadius,				
             int ransacIterations, double thresholdDistance);
			 
/**
 * desc: This function takes a source PCD image an segments out objects found 
 *       on a plane.
 * param: (in) sourceCloud - ptr to input cloud
 *        (out) objectsCloud - returns cloud with objects above plane
 *        (in) minRadius - minimum radius between plane and camera in radians (ex. 0)
 *        (in) maxRadius - maximum radius between plane and camera in radians (ex. 1.57)
 *        (in) ransacIterations - maximum amount of times RANSAC can estimate plane (ex. 1000)
 *        (in) thresholdDistance - range of points considered inliers from plane model (ex 0.01 = 1cm)
 *        (in) minObjectDistance - lowest point of objects above a plane (ex 0.01 = 1cm)
 *        (in) maxObjectDistance - highest point of objects above a plane (ex 0.2 = 20cm)
 * pre-cond: all output variables must be declared before function is called.
 * ret: -1 if a plane could not be found else
 *      -2 if source PCD is empty
 *       0 if succesfull
 */
int getPrism(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,
             pcl::PointCloud<pcl::PointXYZ>::Ptr objectsCloud,                                       	
             double minRadius, double maxRadius,				
             int ransacIterations, double thresholdDistance, 
             float minObjectDistance, float maxObjectDistance);
}


/* wrapper functions */
void log_passthroughFilter(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,
	                       pcl::PointCloud<pcl::PointXYZ>::Ptr filteredCloud,
                               const std::string& pcdFileName, const std::string& flavorText,
	                       const std::string& axis, float minPoint, float maxPoint);
void log_voxelFilter(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,
                     pcl::PointCloud<pcl::PointXYZ>::Ptr filteredCloud,
                     const std::string& pcdFileName, const std::string& flavorText,
                     float leafSize);
void log_removeNoise(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,
                     pcl::PointCloud<pcl::PointXYZ>::Ptr filteredCloud,
                     const std::string& pcdFileName, const std::string& flavorText,
                     int neighborsToAnalyze, double stdDeviation);
void log_getPlane(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,  
                  pcl::PointCloud<pcl::PointXYZ>::Ptr planeCloud,                 
                  pcl::PointCloud<pcl::PointXYZ>::Ptr outliersCloud,              
                  pcl::ModelCoefficients::Ptr coefficients,                       
                  pcl::PointIndices::Ptr indices,
                  const std::string& pcdFileName, 
                  const std::string& flavorText,
                  double minRadius, double maxRadius,				
                  int ransacIterations, double thresholdDistance);
void log_getPrism(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,
                  pcl::PointCloud<pcl::PointXYZ>::Ptr objectsCloud,             
                  const std::string& pcdFileName, const std::string& flavorText,				  
                  double minRadius, double maxRadius,				
                  int ransacIterations, double thresholdDistance, 
                  float minObjectDistance, float maxObjectDistance);
			 

int main(int argc, char** argv)
{
	// initialize variables
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_original (new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_unaltered (new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filtered (new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_objects (new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_planeInliers (new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_planeOutliers (new pcl::PointCloud<pcl::PointXYZ>);             
	pcl::ModelCoefficients::Ptr coefficients (new pcl::ModelCoefficients());                  
	pcl::PointIndices::Ptr indices (new pcl::PointIndices());
	pcl::PCDReader reader;
	pcl::PCDWriter writer;
	int result = 0;  // catch function return
	
	// read point cloud through command line
	if (argc != 2)
	{
		std::cout << "usage: " << argv[0] << " <filename>\n";
		return 0;
	}
	else
	{	// read cloud and display its size
		std::cout << "Reading Point Cloud" << std::endl;
		reader.read(argv[1], *cloud_original);
		#ifdef DEBUG
		std::cout << "size of original cloud: " 
		          << cloud_original->points.size() << " points" << std::endl;
		#endif
	}
	
	//**************************************************************************
	// test passthroughFilter() function
	std::cout << "RUNNING PASSTHROUGH FILTER TESTS" << std::endl;
	log_passthroughFilter(cloud_original, cloud_filtered, "T01_passthrough_01.pcd",
	                      "passthrough filter, custom 1: ","z", -2.5, 0);
	log_passthroughFilter(cloud_original, cloud_filtered, "T01_passthrough_02.pcd",
	                      "passthrough filter, custom 2: ","y", -3.0, 3.0);
	log_passthroughFilter(cloud_original, cloud_filtered, "T01_passthrough_03.pcd", 
	                      "passthrough filter, custom 3: ","x", -3.0, 3.0);
	
	//**************************************************************************
	// test voxelFilter() function
	std::cout << "RUNNING VOXEL FILTER TEST" << std::endl;
	log_voxelFilter(cloud_original, cloud_filtered, "T02_voxel_01.pcd",
	                "voxel filter, custom 1:", 0.01);
	log_voxelFilter(cloud_original, cloud_filtered, "T02_voxel_02.pcd",
                        "voxel filter, custom 2:", 0.02);
	log_voxelFilter(cloud_original, cloud_filtered, "T02_voxel_03.pcd",
                        "voxel filter, custom 3:", 0.04);
	log_voxelFilter(cloud_original, cloud_filtered, "T02_voxel_04.pcd",
                        "voxel filter, custom 4:", 0.08);				
	
	//**************************************************************************
	// test removeNoise() function
	std::cout << "RUNNING NOISE REMOVAL TEST" << std::endl;
	log_removeNoise(cloud_original, cloud_filtered, "T03_noise_01.pcd",
	                "noise removal, custom 1: ", 50, 1.0);
	log_removeNoise(cloud_original, cloud_filtered, "T03_noise_01.pcd",
	                "noise removal, custom 1: ", 100, 1.0);
	log_removeNoise(cloud_original, cloud_filtered, "T03_noise_01.pcd",
	                "noise removal, custom 1: ", 10, 1.0);
	log_removeNoise(cloud_original, cloud_filtered, "T03_noise_01.pcd",
	                "noise removal, custom 1: ", 50, 1.9);
	log_removeNoise(cloud_original, cloud_filtered, "T03_noise_01.pcd",
	                "noise removal, custom 1: ", 50, 0.1);				
	
	//**************************************************************************
	// test getPlane() function
	std::cout << "RUNNING PLANE SEGMENTATION TEST" << std::endl;
	log_getPlane(cloud_original, cloud_planeInliers, cloud_planeOutliers,
	             coefficients, indices, "T04_planeInliers_01.pcd",
	             "plane segmentation, custom 1: ", 0, 1.57, 1000, 0.01);
	log_getPlane(cloud_original, cloud_planeInliers, cloud_planeOutliers,
	             coefficients, indices, "T04_planeInliers_02.pcd",
	             "plane segmentation, custom 2: ", 0, 1.57, 1, 0.01);
	log_getPlane(cloud_original, cloud_planeInliers, cloud_planeOutliers,
	             coefficients, indices, "T04_planeInliers_03.pcd",
	             "plane segmentation, custom 3: ", 0, 1.57, 2000, 0.01);
	log_getPlane(cloud_original, cloud_planeInliers, cloud_planeOutliers,
	             coefficients, indices, "T04_planeInliers_04.pcd",
	             "plane segmentation, custom 4: ", 0, 0.76, 1000, 0.01);
	log_getPlane(cloud_original, cloud_planeInliers, cloud_planeOutliers,
	             coefficients, indices, "T04_planeInliers_05.pcd",
	             "plane segmentation, custom 5: ", 0, 2.09, 1000, 0.01);
	log_getPlane(cloud_original, cloud_planeInliers, cloud_planeOutliers,
	             coefficients, indices, "T04_planeInliers_06.pcd",
	             "plane segmentation, custom 6: ", 0.35, 1.57, 1000, 0.01);
	log_getPlane(cloud_original, cloud_planeInliers, cloud_planeOutliers,
	             coefficients, indices, "T04_planeInliers_07.pcd",
	             "plane segmentation, custom 7: ", 0.79, 1.57, 1000, 0.01);
	log_getPlane(cloud_original, cloud_planeInliers, cloud_planeOutliers,
	             coefficients, indices, "T04_planeInliers_08.pcd",
	             "plane segmentation, custom 8: ", 0, 1.57, 1000, 0.001);
	log_getPlane(cloud_original, cloud_planeInliers, cloud_planeOutliers,
	             coefficients, indices, "T04_planeInliers_09.pcd",
	             "plane segmentation, custom 9: ", 0, 1.57, 1000, 0.01);			 

	//**************************************************************************
	// test getPrism() function
	std::cout << "RUNNING EXTRACT PRISM TEST" << std::endl;
	log_getPrism(cloud_original, cloud_objects, "T05_objects_01.pcd",
	             "polygonal prism, custom 1: ", 0, 1.57, 1000, 0.01, 0.02, 0.2);

	return 0;
}


/* WRAPPER IMPLEMENTATION */
void log_passthroughFilter(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,
	                   pcl::PointCloud<pcl::PointXYZ>::Ptr filteredCloud,
                           const std::string& pcdFileName, const std::string& flavorText,
	                   const std::string& axis, float minPoint, float maxPoint)
{
	pcl::PCDWriter writer;  // for writting pcd to disk

	// enable variables for time logging
	boost::posix_time::ptime time_before_execution;
	boost::posix_time::ptime time_after_execution;
	boost::posix_time::time_duration difference;

	time_before_execution = boost::posix_time::microsec_clock::local_time();  // time before
	c44::passthroughFilter(sourceCloud,filteredCloud, axis, minPoint, maxPoint);  // call filter
	time_after_execution = boost::posix_time::microsec_clock::local_time(); // time after
	writer.write<pcl::PointXYZ> (pcdFileName, *filteredCloud); // write out cloud
	#ifdef DEBUG
	difference = time_after_execution - time_before_execution;  // get execution time
	std::cout << std::setw(5) << difference.total_milliseconds() << ": "
	          << flavorText << filteredCloud->points.size() << " points" << std::endl;
	#endif
}
						   
						   
void log_voxelFilter(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,
                     pcl::PointCloud<pcl::PointXYZ>::Ptr filteredCloud,
                     const std::string& pcdFileName, const std::string& flavorText,
                     float leafSize)
{
	pcl::PCDWriter writer;  // for writting pcd to disk	

	// enable variables for time logging
	boost::posix_time::ptime time_before_execution;
	boost::posix_time::ptime time_after_execution;
	boost::posix_time::time_duration difference;
	
	time_before_execution = boost::posix_time::microsec_clock::local_time();  // time before
	c44::voxelFilter(sourceCloud, filteredCloud, leafSize);  // call voxel filter
	time_after_execution = boost::posix_time::microsec_clock::local_time(); // time after
	writer.write<pcl::PointXYZ> (pcdFileName, *filteredCloud); // write out cloud
	#ifdef DEBUG
	difference = time_after_execution - time_before_execution;  // get execution time
	std::cout << std::setw(5) << difference.total_milliseconds() << ": "
	          << flavorText << filteredCloud->points.size() << " points" << std::endl;
	#endif
}
					 
					 
void log_removeNoise(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,
                     pcl::PointCloud<pcl::PointXYZ>::Ptr filteredCloud,
                     const std::string& pcdFileName, const std::string& flavorText,
                     int neighborsToAnalyze, double stdDeviation)
{
	pcl::PCDWriter writer;  // for writting pcd to disk

	// enable variables for time logging
	boost::posix_time::ptime time_before_execution;
	boost::posix_time::ptime time_after_execution;
	boost::posix_time::time_duration difference;
	
	time_before_execution = boost::posix_time::microsec_clock::local_time();  // time before
	c44::removeNoise(sourceCloud, filteredCloud, neighborsToAnalyze, stdDeviation);  // call noise removal
	time_after_execution = boost::posix_time::microsec_clock::local_time(); // time after
	writer.write<pcl::PointXYZ> (pcdFileName, *filteredCloud); // write out cloud
	#ifdef DEBUG
	difference = time_after_execution - time_before_execution;  // get execution time
	std::cout << std::setw(5) << difference.total_milliseconds() << ": "
	          << flavorText << filteredCloud->points.size() << " points" << std::endl;
	#endif
}
					 
					 
void log_getPlane(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,  
                  pcl::PointCloud<pcl::PointXYZ>::Ptr planeCloud,                 
                  pcl::PointCloud<pcl::PointXYZ>::Ptr outliersCloud,              
                  pcl::ModelCoefficients::Ptr coefficients,                       
                  pcl::PointIndices::Ptr indices,	
                  const std::string& pcdFileName, const std::string& flavorText,
                  double minRadius, double maxRadius,				
                  int ransacIterations, double thresholdDistance)
{
	pcl::PCDWriter writer;  // for writting pcd to disk

	// enable variables for time logging
	boost::posix_time::ptime time_before_execution;
	boost::posix_time::ptime time_after_execution;
	boost::posix_time::time_duration difference;
	
	// create new cloud to hold original cloud
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_unaltered (new pcl::PointCloud<pcl::PointXYZ>);
	
	pcl::copyPointCloud(*sourceCloud, *cloud_unaltered);  // save original cloud
	time_before_execution = boost::posix_time::microsec_clock::local_time();  // time before
	int result = c44::getPlane(cloud_unaltered, planeCloud, outliersCloud, coefficients, 
	                           indices, minRadius, maxRadius, ransacIterations, thresholdDistance);

	time_after_execution = boost::posix_time::microsec_clock::local_time(); // time after
	if(result < 0)  // default settings
	{ 
		std::cout << "ERROR: No plane found" << std::endl;
	}
	else
	{
		difference = time_after_execution - time_before_execution;  // get execution time
		writer.write<pcl::PointXYZ> (pcdFileName, *planeCloud); // write out cloud
		#ifdef DEBUG
		std::cout << std::setw(5) << difference.total_milliseconds() << ": "
		          << flavorText << planeCloud->points.size() << " points" << std::endl;
		#endif
	}	
}
				  
				  
void log_getPrism(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,
                  pcl::PointCloud<pcl::PointXYZ>::Ptr objectsCloud,
                  const std::string& pcdFileName, const std::string& flavorText,
                  double minRadius, double maxRadius,				
                  int ransacIterations, double thresholdDistance, 
                  float minObjectDistance, float maxObjectDistance)
{
	pcl::PCDWriter writer;  // for writting pcd to disk

	// enable variables for time logging
	boost::posix_time::ptime time_before_execution;
	boost::posix_time::ptime time_after_execution;
	boost::posix_time::time_duration difference;
	
	time_before_execution = boost::posix_time::microsec_clock::local_time();  // time before
	int result = c44::getPrism(sourceCloud, objectsCloud, minRadius, maxRadius, 
	                           ransacIterations, thresholdDistance, maxObjectDistance, minObjectDistance);
	time_after_execution = boost::posix_time::microsec_clock::local_time(); // time after
	if(result < 0)
	{
		std::cout << "ERROR: could not find polygonal prism data" << std::endl;
	}
	else
	{
		writer.write<pcl::PointXYZ> (pcdFileName, *objectsCloud); // write out cloud
		#ifdef DEBUG
		difference = time_after_execution - time_before_execution;  // get execution time
		std::cout << std::setw(5) << difference.total_milliseconds() << ": "
		          << flavorText << objectsCloud->points.size() << " points" << std::endl;
		#endif
	}
}
				  

///////////////////////////////////////////////////////////////////////////////
/** 
 * desc:  This function cuts off all points outside a given range
 *        on the field passed the the field parameter.
 * param: (in) sourceCloud - ptr to point cloud 
 *        (out) filteredCloud - ptr to cloud after filtering
 *        (in) axis - axis to filter values passed as 'x', 'y', or 'z'
 *        (in) minPoint - kept points are >= minPoint
 *        (in) maxPoint - kept points are <= maxPoint
 */
void c44::passthroughFilter(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,
                            pcl::PointCloud<pcl::PointXYZ>::Ptr filteredCloud,
                            const std::string& axis, float minPoint, float maxPoint)
{
	pcl::PassThrough<pcl::PointXYZ> pass;
	pass.setInputCloud(sourceCloud);
	pass.setFilterFieldName(axis);
	pass.setFilterLimits(minPoint, maxPoint);
	pass.filter(*filteredCloud);
}


///////////////////////////////////////////////////////////////////////////////
/**
 * desc: This function uses a voxel filter to downsample a PCD file.
 * param: (in) sourceCloud - ptr to cloud to be filtered
 *        (out) filteredCloud - ptr to cloud after filtering
 *        (in) leafSize - size of voxel in cm^3. (ex. 0.01 = 1 cm)
 */
void c44::voxelFilter(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,
                      pcl::PointCloud<pcl::PointXYZ>::Ptr filteredCloud,
                      float leafSize)
{
	pcl::VoxelGrid<pcl::PointXYZ> sor;
	sor.setInputCloud(sourceCloud);
	sor.setLeafSize(leafSize, leafSize, leafSize);
	sor.filter(*filteredCloud);
}


///////////////////////////////////////////////////////////////////////////////
/**
 * desc: This function computes the average distance between each point in a PCD
 *       file and then uses the standard deviation to designate points as outliers.
 *       All outliers are then removed from the resultant point cloud.
 * param: (in) sourceCloud - ptr to input cloud
 *        (out) filteredCloud - ptr to cloud with outliers removed
 *        (in) neighborsToAnalyze - number of nearest neighbors to analyze (ex. 50)
 *        (in) stdDeviation - standard deviation used to find outliers (ex. 1.0)   
 */
void c44::removeNoise(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,
                      pcl::PointCloud<pcl::PointXYZ>::Ptr filteredCloud,
                      int neighborsToAnalyze, double stdDeviation)
{
	pcl::StatisticalOutlierRemoval<pcl::PointXYZ> sor;
	sor.setInputCloud(sourceCloud);
	sor.setMeanK(neighborsToAnalyze);
	sor.setStddevMulThresh(stdDeviation);
	sor.filter(*filteredCloud);
}


///////////////////////////////////////////////////////////////////////////////
/** 
 * desc: This function takes a source point cloud and segments out a plane.
 *       The inliers of the plane, as well as all information needed to
 *       rebuild the plane is returned through the function parameters.
 * param: (in) sourceCloud - ptr to input cloud
 *        (out) inliersCloud - returns inliers of plane as pointcloud 
 *        (out) outliersCloud - returns outliers of plane as pointcloud
 *        (out) coefficients - returns the coefficients of the plane
 *        (out) indices - returns the indices of the plane
 *        (in) minRadius - minimum radius between plane and camera in radians (ex. 0)
 *        (in) maxRadius - maximum radius between plane and camera in radians (ex. 1.57)
 *        (in) ransacIterations - maximum amount of times RANSAC can estimate plane (ex. 1000)
 *        (in) thresholdDistance - range of points considered inliers from plane model (ex 0.01 = 1cm)
 * pre-cond: all output parameters must be declared before function call
 * ret: -1 if a plane could not be found else
 *      -2 if source PCD is empty
 *       0 if succesfull
 */
int c44::getPlane(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,  
                  pcl::PointCloud<pcl::PointXYZ>::Ptr inliersCloud,                 
                  pcl::PointCloud<pcl::PointXYZ>::Ptr outliersCloud,              
                  pcl::ModelCoefficients::Ptr coefficients,                       
                  pcl::PointIndices::Ptr plane_indices,	
                  double minRadius, double maxRadius,				
                  int ransacIterations, double thresholdDistance)
{

	// check that data in cloud is valid, since errors keep occuring
	if(NULL == sourceCloud)
	{
		return -2;
	}
	#ifdef DEBUG
	pcl::PCDWriter writer;
	writer.write<pcl::PointXYZ> ("cloud_before_plane_segmentation.pcd", *sourceCloud, false);
	#endif

	// Create the segmentation object
	pcl::SACSegmentation<pcl::PointXYZ> seg;

	// Optional
	seg.setOptimizeCoefficients(true);

	// Mandatory
	seg.setModelType(pcl::SACMODEL_PLANE);
	seg.setMethodType(pcl::SAC_RANSAC);
	seg.setMaxIterations(ransacIterations);
	seg.setDistanceThreshold(thresholdDistance);
	seg.setRadiusLimits(minRadius, maxRadius);  // TODO: may need to use different function

	// Segment the largest planar component from the remaining cloud
	seg.setInputCloud(sourceCloud);
	seg.segment(*plane_indices, *coefficients);

	// check if a plane was found	
	if (plane_indices->indices.size() == 0)
	{	// No plane was found
		return -1;
	}
	
	// Create the filtering object
	pcl::ExtractIndices<pcl::PointXYZ> extract;

	// Extract the inliers
	extract.setInputCloud(sourceCloud);
	extract.setIndices(plane_indices);
	extract.setNegative(false);
	extract.filter(*inliersCloud);

	#ifdef DEBUG
	std::cerr << "PointCloud representing the planar component: "
	          << inliersCloud->width * inliersCloud->height << " data points."
	          << std::endl;
	#endif

	// Create the filtering object
	extract.setNegative(true);
	extract.filter(*outliersCloud);
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
/**
 * desc: This function takes a source PCD image an segments out objects found 
 *       on a plane.
 * param: (in) sourceCloud - ptr to input cloud
 *        (out) objectsCloud - returns cloud with objects above plane
 *        (in) minRadius - minimum radius between plane and camera in radians (ex. 0)
 *        (in) maxRadius - maximum radius between plane and camera in radians (ex. 1.57)
 *        (in) ransacIterations - maximum amount of times RANSAC can estimate plane (ex. 1000)
 *        (in) thresholdDistance - range of points considered inliers from plane model (ex 0.01 = 1cm)
 *        (in) minObjectDistance - lowest point of objects above a plane (ex 0.01 = 1cm)
 *        (in) maxObjectDistance - highest point of objects above a plane (ex 0.2 = 20cm)
 * pre-cond: all output variables must be declared before function is called.
 * ret: -1 if a plane could not be found else
 *      -2 if source PCD is empty
 *       0 if succesfull
 */
int c44::getPrism(pcl::PointCloud<pcl::PointXYZ>::Ptr sourceCloud,
                  pcl::PointCloud<pcl::PointXYZ>::Ptr objectsCloud,                                       	
                  double minRadius, double maxRadius,				
                  int ransacIterations, double thresholdDistance,
                  float minObjectDistance, float maxObjectDistance)
{
	pcl::ModelCoefficients::Ptr coefficients (new pcl::ModelCoefficients);
	pcl::PointIndices::Ptr plane_indices (new pcl::PointIndices);
	pcl::PointCloud<pcl::PointXYZ>::Ptr inliersCloud (new pcl::PointCloud<pcl::PointXYZ>);
	

	// find plane parameters
	pcl::SACSegmentation<pcl::PointXYZ> seg;
	seg.setOptimizeCoefficients(true);
	seg.setModelType(pcl::SACMODEL_PLANE);
	seg.setMethodType(pcl::SAC_RANSAC);
	seg.setMaxIterations(ransacIterations);
	seg.setDistanceThreshold(thresholdDistance);
	seg.setRadiusLimits(minRadius, maxRadius);  // TODO: may need to use different function
	seg.setInputCloud(sourceCloud);
	seg.segment(*plane_indices, *coefficients);

	// check if a plane was found	
	if (plane_indices->indices.size() == 0)
	{	// No plane was found
		return -1;
	}
	else
	{
		// Create the filtering object
		pcl::ExtractIndices<pcl::PointXYZ> extract;

		// Extract the inliers
		extract.setInputCloud(sourceCloud);
		extract.setIndices(plane_indices);
		extract.filter(*inliersCloud);

		// retrieve the convex hull
		pcl::PointCloud<pcl::PointXYZ>::Ptr convexHull (new pcl::PointCloud<pcl::PointXYZ>);
		pcl::ConvexHull<pcl::PointXYZ> chull;
		chull.setInputCloud (inliersCloud);
		chull.setDimension(2);
		chull.reconstruct(*convexHull);

		// redundant check
		if (chull.getDimension() == 2)
		{
			#ifdef DEBUG
			pcl::PCDWriter writer;
			writer.write<pcl::PointXYZ> ("cloud_convex_hull.pcd", *convexHull, false);
			#endif

			// segment those points that are in the polygonal prism
			pcl::ExtractPolygonalPrismData<pcl::PointXYZ> prism;
			prism.setInputCloud(sourceCloud);
			prism.setInputPlanarHull(convexHull);
			prism.setHeightLimits((float) minObjectDistance, (float) maxObjectDistance);
			pcl::PointIndices::Ptr objectIndices (new pcl::PointIndices);
			prism.segment (*objectIndices);
	
			// get all points retrieved by the hull
			extract.setIndices(objectIndices);
			extract.filter(*objectsCloud);

			#ifdef DEBUG
			writer.write<pcl::PointXYZ> ("cloud_objects.pcd", *objectsCloud, false);
			std::cout << "PRISM: OBJECTS CLOUD" << std::endl;
			#endif
		}
		else
		{	// the chosen hull is not planar
			return -1;
		}
	}
	return 0;
}

