#include "../include/grasp_sampler/Kinematics.h"

kinematics_sovler::kinematics_sovler(YAMLConfig &config):
config_(config)
{
    initModel();
	calcReachability();
}

void kinematics_sovler::initModel()
{
    std::string urdf_absolute_path;
	std::string mod_url = config_.urdf_path;
	if (config_.urdf_path.find("package://") == 0)
	{
		mod_url.erase(0, strlen("package://"));
		size_t pos = mod_url.find("/");
		if (pos == std::string::npos)
		{
			std::cout << "Could not parse package:// format into file:// format" << std::endl;
		}
		std::string package = mod_url.substr(0, pos);
		mod_url.erase(0, pos);
		std::string package_path = ros::package::getPath(package);

		if (package_path.empty())
		{
			std::cout << "Package does not exist" << std::endl;
		}

		urdf_absolute_path =  package_path + mod_url;
	}

	RigidBodyDynamics::Addons::URDFReadFromFile(urdf_absolute_path.c_str(), &rbdl_model_, false, false);

	// Serial Robot DOF = nb_of_joint
	nb_of_joints_ = config_.joint_limit_lower.size();

	end_effector_id_ = rbdl_model_.GetBodyId((config_.chain_end).c_str());
	arm_base_frame_id_ = rbdl_model_.GetBodyId((config_.chain_start).c_str());

	if (rbdl_model_.IsFixedBodyId(end_effector_id_))
	{
		end_effector_com_ = rbdl_model_.mFixedBodies[end_effector_id_ - rbdl_model_.fixed_body_discriminator].mCenterOfMass;
	}
	else
	{
		end_effector_com_ = rbdl_model_.mBodies[end_effector_id_].mCenterOfMass;
	}

	if (rbdl_model_.IsFixedBodyId(arm_base_frame_id_))
	{
		arm_base_frame_pos_ = rbdl_model_.mFixedBodies[arm_base_frame_id_ - rbdl_model_.fixed_body_discriminator].mCenterOfMass;
	}
	else
	{
		arm_base_frame_pos_ = rbdl_model_.mBodies[arm_base_frame_id_].mCenterOfMass;
	}

	std::cout << "Model Loaded" << std::endl;
}

void kinematics_sovler::initializeData(task_assembly::door_open_planner::Request &req)
{
	int nb_of_joint_val = req.current_arm_joint_state.position.size();

	cur_joint_val_ = KDL::JntArray(nb_of_joint_val);

	for(unsigned int jnt_num = 0 ;jnt_num < nb_of_joint_val; jnt_num++)
		cur_joint_val_(jnt_num)= req.current_arm_joint_state.position.at(jnt_num);  //in rad

	// Memory Allocate
	joint_limit_.lower_.resize(nb_of_joints_);
	joint_limit_.lower_rad_.resize(nb_of_joints_);
	joint_limit_.upper_.resize(nb_of_joints_);
	joint_limit_.upper_rad_.resize(nb_of_joints_);

	for (std::vector<int>::size_type i=0;i<config_.joint_limit_lower.size();i++){
		joint_limit_.lower_(i) = config_.joint_limit_lower[i];
		joint_limit_.upper_(i) = config_.joint_limit_upper[i];
	}

	// Deg to rad
	joint_limit_.lower_rad_ = joint_limit_.lower_ / 180.0*M_PI;
	joint_limit_.upper_rad_ = joint_limit_.upper_ / 180.0*M_PI;

	this->initializeIKparam(config_.chain_start, config_.chain_end, urdf_param_);

}

bool kinematics_sovler::initializeIKparam(const std::string& base_link, const std::string& tip_link, const std::string& URDF_param)
{
	ros::NodeHandle node_handle("~");

	std::string xml_string;

	std::string urdf_xml, full_urdf_xml;
	node_handle.param("urdf_xml", urdf_xml, URDF_param);
	node_handle.searchParam(urdf_xml, full_urdf_xml);

	//ROS_DEBUG_NAMED("IK", "Reading xml file from parameter server");
	if (!node_handle.getParam(full_urdf_xml, xml_string))
	{
		std::cout<<"Could not load the xml from parameter server"<<std::endl;
		return false;
	}

	node_handle.param(full_urdf_xml, xml_string, std::string());
	IK_robot_model.initString(xml_string);

	//ROS_DEBUG_STREAM_NAMED("trac_ik", "Reading joints and links from URDF");

	if (!kdl_parser::treeFromUrdfModel(IK_robot_model, IK_tree))
		std::cout<<"Failed to extract kdl tree from xml robot description"<<std::endl;

	if (!IK_tree.getChain(base_link, tip_link, IK_chain))
		std::cout<<"Couldn't find chain"<<std::endl;

	std::vector<KDL::Segment> chain_segs = IK_chain.segments;

	urdf::JointConstSharedPtr joint;

	IK_lb.resize(IK_chain.getNrOfJoints());
	IK_ub.resize(IK_chain.getNrOfJoints());

	uint joint_num = 0;
	for (unsigned int i = 0; i < chain_segs.size(); ++i)
	{
		joint = IK_robot_model.getJoint(chain_segs[i].getJoint().getName());
		if (joint->type != urdf::Joint::UNKNOWN && joint->type != urdf::Joint::FIXED)
		{
			joint_num++;
			float lower, upper;
			int hasLimits;
			if (joint->type != urdf::Joint::CONTINUOUS)
			{
				if (joint->safety)
				{
					lower = std::max(joint->limits->lower, joint->safety->soft_lower_limit);
					upper = std::min(joint->limits->upper, joint->safety->soft_upper_limit);
				}
				else
				{
					lower = joint->limits->lower;
					upper = joint->limits->upper;
				}
				hasLimits = 1;
			}
			else
			{
				hasLimits = 0;
			}
			if (hasLimits)
			{
				IK_lb(joint_num - 1) = lower;
				IK_ub(joint_num - 1) = upper;
			}
			else
			{
				IK_lb(joint_num - 1) = std::numeric_limits<float>::lowest();
				IK_ub(joint_num - 1) = std::numeric_limits<float>::max();
			}
			//ROS_DEBUG_STREAM_NAMED("trac_ik", "IK Using joint " << joint->name << " " << lb(joint_num - 1) << " " << ub(joint_num - 1));
		}
	}
}

Eigen::Matrix4f kinematics_sovler::Frame2Eigen(KDL::Frame &frame)
{
	Eigen::Matrix4f H_trans;
    H_trans.resize(4,4);
    double d[16] = {0,};
    frame.Make4x4(d);

    H_trans(0,0) = d[0];
    H_trans(0,1) = d[1];
    H_trans(0,2) = d[2];
    H_trans(0,3) = d[3];
    H_trans(1,0) = d[4];
    H_trans(1,1) = d[5];
    H_trans(1,2) = d[6];
    H_trans(1,3) = d[7];
    H_trans(2,0) = d[8];
    H_trans(2,1) = d[9];
    H_trans(2,2) = d[10];
    H_trans(2,3) = d[11];
    H_trans(3,0) = d[12];
    H_trans(3,1) = d[13];
    H_trans(3,2) = d[14];
    H_trans(3,3) = d[15];

    return H_trans;
}

Eigen::Matrix4f kinematics_sovler::solveFK()
{
    robot_joint_val_= cur_joint_val_;

    KDL::ChainFkSolverPos_recursive Fksolver = KDL::ChainFkSolverPos_recursive(FK_chain_);

    if(Fksolver.JntToCart(robot_joint_val_,T_BC_Frame_) < 0)
        std::cerr<<"Fk about robot Model Failed!!!!"<<std::endl;

    // T_BO = T_BC * T_CO ---> External Params
	return Frame2Eigen(T_BC_Frame_);
}

bool kinematics_sovler::solveIK(geometry_msgs::Pose _goal_info)
{
	KDL::Frame end_effector_pose;
	KDL::Rotation rot;

	//geometry info to KDL frma info

	end_effector_pose.p(0) = _goal_info.position.x;
	end_effector_pose.p(1) = _goal_info.position.y;
	end_effector_pose.p(2) = _goal_info.position.z;

	end_effector_pose.M = rot;

	/////////////// setup track-ik //////////////////
	TRAC_IK::TRAC_IK tracik_solver(config_.chain_start, config_.chain_end, urdf_param_,100,eps);

	bool valid = tracik_solver.getKDLChain(IK_chain);

	if (!valid)
	{
		std::cout<<"There was no valid KDL chain found"<<std::endl;
		return false;
	}

	valid = tracik_solver.getKDLLimits(IK_lb,IK_ub);

	if (!valid)
	{
		std::cout<<"There were no valid KDL joint limits found"<<std::endl;
		return false;
	}

	KDL::JntArray nominal(IK_chain.getNrOfJoints());

	int rc;
	double total_time = 0;
	uint success = 0;
	bool solved = true;
	bool update = false;

	nominal = cur_joint_val_;

	rc = tracik_solver.CartToJnt(nominal, end_effector_pose, IK_result_);

	if (rc <= 0)
	{
		std::cout<<"IK solution Not Exist!!!"<<std::endl;
		return false;
	}
	return true;
}

void kinematics_sovler::calcReachability()
{
	std::cout<<"calculating reachability....."<<std::endl;
	std::cout<<"sampling manipualtor configuration....."<<std::endl;

	// Sphere 
	// 1. calc L2 norm
	// 2. if robot base to grasp pose Euclidean dist > L2 norm IK not Exist  

	// reachability imagine 
	// 1. sampling points from random joint variable
	// 2. make 3d polygon from points 
	// 3. show grasp pose is in 3d polygon area 
	
	// sampling random configuration
	KDL::JntArray nominal(IK_chain.getNrOfJoints());
	std::vector<double> R;

	for (int i = 0; i < IK_chain.getNrOfJoints(); i++)		//Sampling Random 시작 위치
	{
		double jointrange = joint_limit_.upper_rad_(i) - joint_limit_.lower_rad_(i); // angle
		double r = ((double)rand() / (double)RAND_MAX) * jointrange;			//normalized Random value
		R.push_back(joint_limit_.lower_rad_(i) + r);
	}

	for (size_t j = 0; j < nominal.data.size(); j++)
	{
		nominal(j) = R[j];
	}
}