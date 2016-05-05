//
//  Geometries.hpp
//  PCLTestbench
//
//  Created on 2/10/16.
//

#ifndef RigidBody_h
#define RigidBody_h
#include <stdio.h>
#include <pcl/ModelCoefficients.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/filters/passthrough.h>
#include <pcl/features/normal_3d.h>
#include <pcl/sample_consensus/method_types.h>
#include <pcl/sample_consensus/model_types.h>
#include <pcl/common/transforms.h>
#include <pcl/common/common.h>
#include <pcl/features/moment_of_inertia_estimation.h>
#include <pcl/features/vfh.h>
#include <stdio.h>
#include <pcl/features/cvfh.h>
#include <pcl/surface/mls.h>
#include <pcl/features/our_cvfh.h>
#include <pcl/features/grsd.h>
#include <pcl/features/esf.h>
#include <bounding_box_utils.hpp>

typedef pcl::Histogram<90> CRH90;


enum HistogramType{
  VFH,
  CVFH,
  OURCVFH,
  ESF,
  GRSD
};

//struct which wraps a point cloud with a bit of extra functionality
namespace c44{
  using namespace pcl;
  
  static const HistogramType default_est_method = HistogramType::GRSD;
  struct RigidBody{
    
  public:
        
    const ModelCoefficients coefficients;
    Cloud3D::Ptr point_cloud;
    PointCloud<Normal>::Ptr normal_cloud;
    
    RigidBody(ModelCoefficients mc,
              Cloud3D::Ptr cloud,
              PointCloud<Normal>::Ptr normals) :
    coefficients(mc), point_cloud(cloud), normal_cloud(normals) {};
    RigidBody(PointCloud<PointXYZ>::Ptr cloud) : point_cloud(cloud){};
    
  };

  template <HistogramType H = default_est_method>
  struct RigidBodyWithHistogram;
  
  template<>
  struct RigidBodyWithHistogram<VFH> : public RigidBody{
    typedef VFHSignature308 signature_t;
    static const string fieldName;
    static const string fileExt;
    #include <rigid_body_shared_template.h>
    
    PointCloud<signature_t>::Ptr computeDescriptor() const{
      // Object for storing the normals.
      pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>);
      // Object for storing the VFH descriptor.
      pcl::PointCloud<pcl::VFHSignature308>::Ptr descriptor(new pcl::PointCloud<pcl::VFHSignature308>);
      
      // Note: you should have performed preprocessing to cluster out the object
      // from the cloud, and save it to this individual file.
      
      // Estimate the normals.
      pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> normalEstimation;
      normalEstimation.setInputCloud(point_cloud);
      normalEstimation.setRadiusSearch(0.03);
      pcl::search::KdTree<pcl::PointXYZ>::Ptr kdtree(new pcl::search::KdTree<pcl::PointXYZ>);
      normalEstimation.setSearchMethod(kdtree);
      normalEstimation.compute(*normals);
      
      // VFH estimation object.
      pcl::VFHEstimation<pcl::PointXYZ, pcl::Normal, pcl::VFHSignature308> vfh;
      vfh.setInputCloud(point_cloud);
      vfh.setInputNormals(normals);
      vfh.setSearchMethod(kdtree);
      // Optionally, we can normalize the bins of the resulting histogram,
      // using the total number of points.
      vfh.setNormalizeBins(true);
      // Also, we can normalize the SDC with the maximum size found between
      // the centroid and any of the cluster's points.
      vfh.setNormalizeDistance(false);
      
      vfh.compute(*descriptor);
      
      return descriptor;
    }
    
    
    
  };
  
  template<>
  struct RigidBodyWithHistogram<CVFH> : public RigidBody{
    typedef VFHSignature308 signature_t;
    static const string fieldName;
    static const string fileExt;

    #include <rigid_body_shared_template.h>
    
    PointCloud<signature_t>::Ptr computeDescriptor() const{
      
      // Object for storing the normals.
      pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>);
      // Object for storing the CVFH descriptors.
      pcl::PointCloud<pcl::VFHSignature308>::Ptr descriptor(new pcl::PointCloud<pcl::VFHSignature308>);
      
      // Note: you should have performed preprocessing to cluster out the object
      // from the cloud, and save it to this individual file.
      
      
      // Estimate the normals.
      pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> normalEstimation;
      normalEstimation.setInputCloud(point_cloud);
      normalEstimation.setRadiusSearch(0.03);
      pcl::search::KdTree<pcl::PointXYZ>::Ptr kdtree(new pcl::search::KdTree<pcl::PointXYZ>);
      normalEstimation.setSearchMethod(kdtree);
      normalEstimation.compute(*normals);
      
      // CVFH estimation object.
      pcl::CVFHEstimation<pcl::PointXYZ, pcl::Normal, pcl::VFHSignature308> cvfh;
      cvfh.setInputCloud(point_cloud);
      cvfh.setInputNormals(normals);
      cvfh.setSearchMethod(kdtree);
      // Set the maximum allowable deviation of the normals,
      // for the region segmentation step.
      cvfh.setEPSAngleThreshold(5.0 / 180.0 * M_PI); // 5 degrees.
                                                     // Set the curvature threshold (maximum disparity between curvatures),
                                                     // for the region segmentation step.
      cvfh.setCurvatureThreshold(1.0);
      // Set to true to normalize the bins of the resulting histogram,
      // using the total number of points. Note: enabling it will make CVFH
      // invariant to scale just like VFH, but the authors encourage the opposite.
      cvfh.setNormalizeBins(true);
      
      cvfh.compute(*descriptor);
      return descriptor;
    }
    
  };
  
  template<>
  struct RigidBodyWithHistogram<ESF> : public RigidBody{
  public:
    typedef ESFSignature640 signature_t;
    static const string fieldName;
    static const string fileExt;

    
    #include <rigid_body_shared_template.h>
    
    PointCloud<signature_t>::Ptr computeDescriptor() const{
      pcl::PointCloud<signature_t>::Ptr descriptor(new PointCloud<signature_t>);
      
      // Note: you should have performed preprocessing to cluster out the object
      // from the cloud, and save it to this individual file.
      
      // Read a PCD file from disk.
      
      // ESF estimation object.
      pcl::ESFEstimation<PointXYZ, ESFSignature640> esf;
      esf.setInputCloud(point_cloud);
      
      esf.compute(*descriptor);
      return descriptor;
    }
    
    
  };
  
  template<>
  struct RigidBodyWithHistogram<GRSD> : public RigidBody{
  public:
    
    typedef GRSDSignature21 signature_t;
    static const string fieldName;
    static const string fileExt;

    
    #include <rigid_body_shared_template.h>
    PointCloud<signature_t>::Ptr
    computeDescriptor() const{
      pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>);
      // Object for storing the GRSD descriptors for each point.
      pcl::PointCloud<pcl::GRSDSignature21>::Ptr descriptors(new pcl::PointCloud<pcl::GRSDSignature21>());
      
      
      // Note: you would usually perform downsampling now. It has been omitted here
      // for simplicity, but be aware that computation can take a long time.
      
      // Estimate the normals.
      pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> normalEstimation;
      normalEstimation.setInputCloud(point_cloud);
      normalEstimation.setRadiusSearch(0.03);
      pcl::search::KdTree<pcl::PointXYZ>::Ptr kdtree(new pcl::search::KdTree<pcl::PointXYZ>);
      normalEstimation.setSearchMethod(kdtree);
      normalEstimation.compute(*normals);
      
      // GRSD estimation object.
      GRSDEstimation<pcl::PointXYZ, pcl::Normal, pcl::GRSDSignature21> grsd;
      grsd.setInputCloud(RigidBody::point_cloud);
      grsd.setInputNormals(normals);
      grsd.setSearchMethod(kdtree);
      // Search radius, to look for neighbors. Note: the value given here has to be
      // larger than the radius used to estimate the normals.
      grsd.setRadiusSearch(0.05);
      
      grsd.compute(*descriptors);
      return descriptors;
      
    }
    
    
  };


  struct Plane : public RigidBody{
  public:
    const PointIndices::Ptr inliers;
    Plane(ModelCoefficients mc,
          Cloud3D::Ptr points,
          PointCloud<Normal>::Ptr normals,
          PointIndices::Ptr _inliers) :
    RigidBody(mc,points,normals), inliers(_inliers){}
  };



  struct GraspableObject : RigidBody{
  public:
    GraspableObject(ModelCoefficients mc,
                    Cloud3D::Ptr points,
                    PointCloud<Normal>::Ptr normals) :
    RigidBody(mc,points,normals){}
    
    BoundingBox getBoundingBox() const;
    
  };
}
#endif