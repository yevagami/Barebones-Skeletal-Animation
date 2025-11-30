#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <unordered_map>
#include <iostream>
#include <SDL.h>
#include <cstdlib>
#include <glew.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stack>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "gui.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <SDL_events.h>

//#include "MemoryMap.h"

/// This is all in 1 file so you don't have to hunt through different files
/// It is tempting to do that, but by leaving it out in the open you can get the big picture 
/// If you want to abstract it or add more features later be my guest.
/// You have to start somewhere, goodluck on your journey
/// -Adriel

/// Sources
/// hasinaxp - this is the basis for this code (https://github.com/hasinaxp/skeletal_animation-_assimp_opengl)
/// https://learnopengl.com/Guest-Articles/2020/Skeletal-Animation

//Mostly to convert to different data types and creating the shader program + create the window
#pragma region HELPER_FUNCTIONS
typedef unsigned int uint;
typedef unsigned char byte;
 
///Note:
/// If you want to convert the matrices to your own math class
/// Know that glm matrices are column major
/// so it goes 
/// 1 4 7
/// 2 5 8
/// 3 6 9
/// 
/// Assimp store their matrices in row major 
/// so it goes
/// 1 2 3
/// 4 5 6
/// 7 8 9

inline glm::mat4 assimpToGlmMatrix(aiMatrix4x4 mat) {
	glm::mat4 m = glm::mat4(0.0f);
	m[0] = glm::vec4(mat.a1, mat.b1, mat.c1, mat.d1);
	m[1] = glm::vec4(mat.a2, mat.b2, mat.c2, mat.d2);
	m[2] = glm::vec4(mat.a3, mat.b3, mat.c3, mat.d3);
	m[3] = glm::vec4(mat.a4, mat.b4, mat.c4, mat.d4);
	return m;
}

inline glm::vec3 assimpToGlmVec3(aiVector3D vec) {
	return glm::vec3(vec.x, vec.y, vec.z);
}

inline glm::quat assimpToGlmQuat(aiQuaternion quat) {
	glm::quat q;
	q.x = quat.x;
	q.y = quat.y;
	q.z = quat.z;
	q.w = quat.w;

	return q;
}

inline unsigned int createShader(const char* vertexStr, const char* fragmentStr) {
	int success;
	char info_log[512];
	uint
		program = glCreateProgram(),
		vShader = glCreateShader(GL_VERTEX_SHADER),
		fShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vShader, 1, &vertexStr, 0);
	glCompileShader(vShader);
	glGetShaderiv(vShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vShader, 512, 0, info_log);
		std::cout << "vertex shader compilation failed!\n" << info_log << std::endl;
	}
	glShaderSource(fShader, 1, &fragmentStr, 0);
	glCompileShader(fShader);
	glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fShader, 512, 0, info_log);
		std::cout << "fragment shader compilation failed!\n" << info_log << std::endl;
	}

	glAttachShader(program, vShader);
	glAttachShader(program, fShader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(program, 512, 0, info_log);
		std::cout << "program linking failed!\n" << info_log << std::endl;
	}
	glDetachShader(program, vShader);
	glDeleteShader(vShader);
	glDetachShader(program, fShader);
	glDeleteShader(fShader);

	return program;
}

inline SDL_Window* initWindow(int& windowWidth, int& windowHeight) {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_LoadLibrary(NULL);

	//window
	SDL_Window* window = SDL_CreateWindow("skin Animation",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		640, 480,
		SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);


	SDL_GLContext context = SDL_GL_CreateContext(window);

	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	SDL_GL_MakeCurrent(window, context);
	GLenum err = glewInit();
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);
	glViewport(0, 0, windowWidth, windowHeight);
	glClearColor(1.0, 0.0, 0.4, 1.0);
	glEnable(GL_DEPTH_TEST);
	SDL_ShowWindow(window);
	SDL_GL_SetSwapInterval(1);

	return window;
}

#pragma endregion

#pragma region SHADERS
const char* vertexShaderSource = R"(
	#version 440 core
	layout (location = 0) in vec3 position; 
	layout (location = 1) in vec3 normal;
	layout (location = 2) in vec2 uv;
	layout (location = 3) in vec4 boneIds;
	layout (location = 4) in vec4 boneWeights;

	out vec2 tex_cord;
	out vec3 v_normal;
	out vec3 v_pos;
	out vec4 bw;

	uniform mat4 bone_transforms[100];
	uniform mat4 view_projection_matrix;
	uniform mat4 model_matrix;

	void main()
	{
		bw = vec4(0);
		if(int(boneIds.x) == 1)
		bw.z = boneIds.x;
		//boneWeights = normalize(boneWeights);
		mat4 boneTransform  =  mat4(0.0);
		boneTransform  +=    bone_transforms[int(boneIds.x)] * boneWeights.x;
		boneTransform  +=    bone_transforms[int(boneIds.y)] * boneWeights.y;
		boneTransform  +=    bone_transforms[int(boneIds.z)] * boneWeights.z;
		boneTransform  +=    bone_transforms[int(boneIds.w)] * boneWeights.w;
		vec4 pos = boneTransform * vec4(position, 1.0);
		gl_Position = view_projection_matrix * model_matrix * pos;
		v_pos = vec3(model_matrix * boneTransform * pos);
		tex_cord = uv;
		v_normal = mat3(transpose(inverse(model_matrix * boneTransform))) * normal;
		v_normal = normalize(v_normal);
	}

	)";
const char* fragmentShaderSource = R"(
	#version 440 core

	in vec2 tex_cord;
	in vec3 v_normal;
	in vec3 v_pos;
	in vec4 bw;
	out vec4 color;

	uniform sampler2D diff_texture;

	vec3 lightPos = vec3(0.2, 1.0, -3.0);
	
	void main()
	{
		vec3 lightDir = normalize(lightPos - v_pos);
		float diff = max(dot(v_normal, lightDir), 0.2);
		vec3 dCol = diff * texture(diff_texture, tex_cord).rgb; 
		color = vec4(dCol, 1);
	}
	)";
#pragma endregion

#pragma region STRUCTS
//Vertex
struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
	glm::vec4 boneIds = glm::vec4(0);
	glm::vec4 boneWeights = glm::vec4(0.0f);
};

//Singular bone
struct Bone  {
	int id = 0; // position of the bone in the final array
	std::string name = "";
	glm::mat4 offset = glm::mat4(1.0f);
	std::vector<Bone> children = {};
};

//Animation track of a single bone contains translation, rotation, scale
struct BoneTransformTrack {
	std::vector<float> positionTimestamps = {};
	std::vector<float> rotationTimestamps = {};
	std::vector<float> scaleTimestamps = {};

	std::vector<glm::vec3> positions = {};
	std::vector<glm::quat> rotations = {};
	std::vector<glm::vec3> scales = {};
};

//Animation
struct Animation {
	float duration = 0.0f; //The duration in ticks
	float ticksPerSecond = 1.0f; //Update rate in a second
	float animationTime = 0.0f; //How much time has passed since this animation was played (in seconds)
	unsigned int boneCount = 0;
	std::unordered_map<std::string, BoneTransformTrack> boneTransforms = {}; //Final bone map
};

//Mesh
struct Mesh {
	std::vector<Vertex> vertices = {};
	std::vector<uint> indices = {};
	unsigned int boneCount = 0;
	Bone skeleton; //The root node of the skeleton tree
	//std::unordered_map<std::string, std::pair<int, glm::mat4>> boneInfoTable;//For debugging purposes, delete later
	unsigned int vao = 0;
	unsigned int textureID;
};

//Model
struct Model{
	std::vector<Mesh> meshes;
	std::vector<Animation> animations;
	glm::mat4 modelMatrix;
};

#pragma endregion

//You probably want to look here
#pragma region SKELETON_FUNCTIONS
//Forward delcaration ignore this
bool readSkeleton(Bone& boneOutput, aiNode* node, std::unordered_map<std::string, std::pair<int, glm::mat4>>& boneInfoTable);
unsigned int createVertexArray(std::vector<Vertex>& vertices, std::vector<uint> indices);

//Loading the meshes and outputing them into a vector
void loadMesh(const aiScene* scene, std::vector<Mesh>& outputMesh) {
	int meshCount = scene->mNumMeshes;

	for (int c = 0; c < meshCount; c++) {
		//Get the mesh from the scene
		aiMesh* mesh = scene->mMeshes[c];
		//The struct that we are pushing into the vector
		Mesh newMesh = {}; 

		MEMORY_CHUNKS::Print();

		//load position, normal, uv
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			//process position 
			Vertex vertex;
			glm::vec3 vector;
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.position = vector;

			//process normal
			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.normal = vector;

			//process uv
			glm::vec2 vec;
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vertex.uv = vec;

			vertex.boneIds = glm::ivec4(0);
			vertex.boneWeights = glm::vec4(0.0f);

			newMesh.vertices.push_back(vertex);
		}

		//Initializing the bone table
		//we're storing the bone names as well as the bone index and the offset matrix
		std::unordered_map<std::string, std::pair<int, glm::mat4>> boneInfo = {};

		//boneCounts is a vector that corespond to each vertices and how many bones are connected to it
		//For instance: bonCounts[0] = 3 means that vertex 0 has 3 bones that affect it
		std::vector<unsigned int> boneCounts;
		boneCounts.resize(newMesh.vertices.size(), 0);  //Resizing the vector in advanced
		newMesh.boneCount = mesh->mNumBones;

		//Giving the bone IDs and Weights to each Vertex in the mesh
		for (unsigned int i = 0; i < newMesh.boneCount; i++) {
			aiBone* bone = mesh->mBones[i];
			//Converting it to glm::mat4
			//If you wanna use your own math library, you have to make your own function
			glm::mat4 m = assimpToGlmMatrix(bone->mOffsetMatrix);
			boneInfo[bone->mName.C_Str()] = { i, m };

			//Looping through the bones that are attached to this particular vertex 
			for (int j = 0; j < bone->mNumWeights; j++) {

				unsigned int id = bone->mWeights[j].mVertexId;
				float weight = bone->mWeights[j].mWeight;

				//This looks weird I know 
				//But tldr we're going through the bones that are attached to this vertex
				//Incrementing it by 1 and storing it back to the corresponding vertex id
				boneCounts[id]++;
				switch (boneCounts[id]) {
				case 1:
					newMesh.vertices[id].boneIds.x = i;
					newMesh.vertices[id].boneWeights.x = weight;
					break;
				case 2:
					newMesh.vertices[id].boneIds.y = i;
					newMesh.vertices[id].boneWeights.y = weight;
					break;
				case 3:
					newMesh.vertices[id].boneIds.z = i;
					newMesh.vertices[id].boneWeights.z = weight;
					break;
				case 4:
					newMesh.vertices[id].boneIds.w = i;
					newMesh.vertices[id].boneWeights.w = weight;
					break;
				default:
					//std::cout << "err: unable to allocate bone to vertex" << std::endl;
					break;
				}
			}
		}

		//Normalize the weights so the sum is 1 between all 4 possible bone weights
		for (int i = 0; i < newMesh.vertices.size(); i++) {
			glm::vec4& boneWeights = newMesh.vertices[i].boneWeights;
			float totalWeight = boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w;
			if (totalWeight > 0.0f) {
				newMesh.vertices[i].boneWeights = glm::vec4(
					boneWeights.x / totalWeight,
					boneWeights.y / totalWeight,
					boneWeights.z / totalWeight,
					boneWeights.w / totalWeight
				);
			}
		}

		//loading the indices
		for (int i = 0; i < mesh->mNumFaces; i++) {
			aiFace& face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				newMesh.indices.push_back(face.mIndices[j]);
		}

		//Creating the bone hirerchy
		readSkeleton(newMesh.skeleton, scene->mRootNode, boneInfo);

		//Creating the VAOs
		newMesh.vao = createVertexArray(newMesh.vertices, newMesh.indices);

		//Adding the mesh to the output
		outputMesh.push_back(newMesh);
	}
}

//This is a recursive function that goes through all the nodes and creating a hierarchy tree from it
bool readSkeleton(Bone& boneOutput, aiNode* node, std::unordered_map<std::string, std::pair<int, glm::mat4>>& boneInfoTable) {
	
	if (boneInfoTable.find(node->mName.C_Str()) != boneInfoTable.end()) { //if the node is actually in the bone table
		boneOutput.name = node->mName.C_Str();
		boneOutput.id = boneInfoTable[boneOutput.name].first;
		boneOutput.offset = boneInfoTable[boneOutput.name].second;

		for (int i = 0; i < node->mNumChildren; i++) {
			Bone child;
			readSkeleton(child, node->mChildren[i], boneInfoTable);
			boneOutput.children.push_back(child);
		}
		return true;
	}
	else { // find bones in children
		for (int i = 0; i < node->mNumChildren; i++) {
			if (readSkeleton(boneOutput, node->mChildren[i], boneInfoTable)) {
				return true;
			}
		}
	}
	return false;
}

//Loading the animation stuff
void loadAnimation(const aiScene* scene, std::vector<Animation>& outputAnimation) {
	int animCount = scene->mNumAnimations;

	for (int i = 0; i < animCount; i++) {
		//Loading the animation
		aiAnimation* anim = scene->mAnimations[i];
		Animation newAnimation = {};

		if (anim->mTicksPerSecond != 0.0f)
			newAnimation.ticksPerSecond = anim->mTicksPerSecond;
		else
			newAnimation.ticksPerSecond = 1;

		newAnimation.duration = anim->mDuration * anim->mTicksPerSecond;
		newAnimation.boneTransforms = {};

		//load positions rotations and scales for each bone
		//each channel represents each bone
		for (int i = 0; i < anim->mNumChannels; i++) {
			aiNodeAnim* channel = anim->mChannels[i];
			BoneTransformTrack track;
			for (int j = 0; j < channel->mNumPositionKeys; j++) {
				track.positionTimestamps.push_back(channel->mPositionKeys[j].mTime);
				track.positions.push_back(assimpToGlmVec3(channel->mPositionKeys[j].mValue));
			}
			for (int j = 0; j < channel->mNumRotationKeys; j++) {
				track.rotationTimestamps.push_back(channel->mRotationKeys[j].mTime);
				track.rotations.push_back(assimpToGlmQuat(channel->mRotationKeys[j].mValue));

			}
			for (int j = 0; j < channel->mNumScalingKeys; j++) {
				track.scaleTimestamps.push_back(channel->mScalingKeys[j].mTime);
				track.scales.push_back(assimpToGlmVec3(channel->mScalingKeys[j].mValue));

			}
			newAnimation.boneTransforms[channel->mNodeName.C_Str()] = track;
			newAnimation.boneCount++;
		}

		outputAnimation.push_back(newAnimation);
	}

}

//Passing the vertex data into the GPU
unsigned int createVertexArray(std::vector<Vertex>& vertices, std::vector<uint> indices) {
	uint
		vao = 0,
		vbo = 0,
		ebo = 0;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, boneIds));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, boneWeights));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint), &indices[0], GL_STATIC_DRAW);
	glBindVertexArray(0);
	return vao;
}

//Using stb_load to load in textures
//Right now I haven't figured out a way to read the texture embeded in the file
//So for now you still have to read it externally
uint createTexture(std::string filepath) {
	uint textureId = 0;
	int width, height, nrChannels;
	byte* data = stbi_load(filepath.c_str(), &width, &height, &nrChannels, 4);
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);
	return textureId;
}

//Given a list of keyframe timestamps
//We iterate through and return the appropriate start and end key frames
//start----x---------end x is the sample time aka dt 
//We then make sure to convert it to 0-1 which we store in the float component of the pair
//This is so we can interpolate between the two keyframes
std::pair<uint, float> getTimeFraction(std::vector<float>& times, float& dt) {
	//Careful when using Unsigned int
	//It is tempting, but if you are checking if an index is valid (-1 or larger than count)
	//Then it might not be a good idea
	int segment = 0;
	//Safeguard incase there is only 1 segement/times
	while (dt > times[segment] && segment < times.size() - 1) {
		segment++;
	}

	float frac;
	if (segment - 1 >= 0) {
		float start = times[segment - 1];
		float end = times[segment];
		frac = (dt - start) / (end - start);
	}
	else {
		frac = times[segment];
	}
	return { segment, frac };
}

//Another recursive function
//This function goes through all the bones and calculates the final transform matrices stored in the output vector
//This vector is then passed into the vertex shader
void getPose(Animation& animation, Bone& skeleton, float dt, std::vector<glm::mat4>& output, glm::mat4& parentTransform, glm::mat4& globalInverseTransform) {
	BoneTransformTrack& btt = animation.boneTransforms[skeleton.name];
	dt = fmod(dt, animation.duration);
	std::pair<int, float> fp;

	glm::vec3 position = glm::vec3();
	if (btt.positions.size() > 0) {
		fp = getTimeFraction(btt.positionTimestamps, dt);

		if (fp.first - 1 >= 0) {
			glm::vec3 position1 = btt.positions[fp.first - 1];
			glm::vec3 position2 = btt.positions[fp.first];

			position = glm::mix(position1, position2, fp.second);
		}
		else {
			position = btt.positions[0];
		}
	}

	//calculate interpolated rotation
	glm::quat rotation = glm::quat(1, 0, 0, 0);
	if (btt.rotations.size() > 0) {
		fp = getTimeFraction(btt.rotationTimestamps, dt);

		if (fp.first - 1 >= 0) {
			glm::quat rotation1 = btt.rotations[fp.first - 1];
			glm::quat rotation2 = btt.rotations[fp.first];

			rotation = glm::slerp(rotation1, rotation2, fp.second);
		}
		else {
			rotation = btt.rotations[0];
		}

	}

	//calculate interpolated scale
	glm::vec3 scale = glm::vec3(1, 1, 1);
	if (btt.scales.size() > 0) {
		fp = getTimeFraction(btt.scaleTimestamps, dt);

		if (fp.first - 1 >= 0) {
			glm::vec3 scale1 = btt.scales[fp.first - 1];
			glm::vec3 scale2 = btt.scales[fp.first];

			scale = glm::mix(scale1, scale2, fp.second);
		}
		else {
			scale = btt.scales[0];
		}
	}

	glm::mat4 positionMat = glm::mat4(1.0), scaleMat = glm::mat4(1.0);

	//Calculate localTransform
	positionMat = glm::translate(positionMat, position);
	glm::mat4 rotationMat = glm::toMat4(rotation);
	scaleMat = glm::scale(scaleMat, scale);
	glm::mat4 localTransform = positionMat * rotationMat * scaleMat;
	glm::mat4 globalTransform = parentTransform * localTransform;

	//If you remember my presentation you would remember the issue with phantom bones
	//This is a hacky fix that just ignores bones that don't have names
	if (skeleton.name != "") {
		output[skeleton.id] = globalInverseTransform * globalTransform * skeleton.offset;
	}

	//update values for children bones
	for (Bone& child : skeleton.children) {
		getPose(animation, child, dt, output, globalTransform, globalInverseTransform);
	}
}

#pragma endregion


int main(int argc, char *argv[]) {

	//Creating the window
	int windowWidth, windowHeight;
	SDL_Window* window = initWindow(windowWidth, windowHeight);
	bool isRunning = true;


	//Creating imgui
	GUI::Init(window, SDL_GL_GetCurrentContext());

	//Loading the file
	//Note: the flags are a bit much I know but these are the ones that work for me
	Assimp::Importer importer;
	const char* filePath = "assets/dancing.gltf";
	const aiScene* scene = importer.ReadFile(filePath, 
		aiProcess_Triangulate | //Makes sure that the vertices are in groups of 3
		aiProcess_LimitBoneWeights| //Limits the amount of bones that can affect a vertex up to 4. You usually don't need any more
		aiProcess_ConvertToLeftHanded | //Converts the coordinates to left hand coordinates
		aiProcess_SortByPType | 
		aiProcess_SplitLargeMeshes | 
		aiProcess_CalcTangentSpace | 
		aiProcess_OptimizeMeshes | 
		aiProcess_GenUVCoords | 
		aiProcess_FindInvalidData | 
		aiProcess_FindDegenerates | 
		aiProcess_ValidateDataStructure | 
		aiProcess_FindInstances 
	);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
	}

	//Creating the model and loading in the meshes & animations
	Model dancingRobot = {};
	loadMesh(scene, dancingRobot.meshes);
	loadAnimation(scene, dancingRobot.animations);

	//We inverse the root node transformation for the outputPose calculation
	//I don't recommend you change this unless you know what you are doing
	glm::mat4 globalInverseTransform = assimpToGlmMatrix(scene->mRootNode->mTransformation);
	globalInverseTransform = glm::inverse(globalInverseTransform);
	
	//Setting the model matrix
	glm::mat4 modelMatrix(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -100.0f, -250.0f));
	modelMatrix = glm::rotate(modelMatrix, 3.14f, glm::vec3(0, 1, 0)); //Fyi the angle is in radians, so 3.14f rad is about 180 deg
	dancingRobot.modelMatrix = modelMatrix;

	//Creating the shader
	uint shader = createShader(vertexShaderSource, fragmentShaderSource);

	//Get all shader uniform locations
	uint viewProjectionMatrixLocation = glGetUniformLocation(shader, "view_projection_matrix");
	uint modelMatrixLocation = glGetUniformLocation(shader, "model_matrix");
	uint boneMatricesLocation = glGetUniformLocation(shader, "bone_transforms");
	uint textureLocation = glGetUniformLocation(shader, "diff_texture"); 

	//Initialize projection and view matrix
	glm::mat4 projectionMatrix = glm::perspective(1.0f, (float)windowWidth / windowHeight, 0.01f, 1000.0f);

	glm::mat4 viewMatrix = glm::lookAt(
		glm::vec3(0.0f, 0.0f, 0.0f) //Position of the eye
		,glm::vec3(0.0f, 0.0f, -5.0f), //Where is it looking at?
		glm::vec3(0, 1, 0)); //What is up?

	glm::mat4 viewProjectionMatrix = projectionMatrix * viewMatrix;

	//Setting up for deltaTime
	float deltaTime = 0.0f;
	float lastTickTime = 0.0f;

	/// Main Loop
	while (isRunning) {
		/// Input
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT)
				isRunning = false;
			// (Where your code calls SDL_PollEvent())
			GUI::ProcessEvent(&ev); // Forward your event to backend
			// (You should discard mouse/keyboard messages in your game/engine when io.WantCaptureMouse/io.WantCaptureKeyboard are set.)
		}

		/// Update
		//Updating deltaTime
		float elapsedTime = (float)SDL_GetTicks() / 1000;
		deltaTime = elapsedTime - lastTickTime;
		lastTickTime = elapsedTime;

		//Updating the animation time
		for (int i = 0; i < dancingRobot.animations.size(); i++) {
			float tickRate = dancingRobot.animations[i].ticksPerSecond;
			float duration = dancingRobot.animations[i].duration / tickRate; //Duration in seconds

			dancingRobot.animations[i].animationTime += deltaTime * tickRate;
			
			//if we are passed it's duration just loop it from the start
			if (dancingRobot.animations[i].animationTime > duration) {
				dancingRobot.animations[i].animationTime = 0.0f;
			}
		}

		GUI::StartRendering();

		/// Rendering
		//Clearing the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shader);

		//For this demo there is only 1 animation for this mode
		Animation& currentAnim = dancingRobot.animations[0];

		//Note: 
		//There is a bug with multiple meshes and 1 skeleton that I haven't figured out a fix yet
		//I suspect that it has something to do with mismatched bones
		//For now mesh 1 works well, but mesh 0 is kinda screwy
		for (int i = 1; i < dancingRobot.meshes.size(); i++) {
			std::vector<glm::mat4> currentPose = {};
			currentPose.resize(currentAnim.boneCount, glm::mat4(1.0f));

			glm::mat4 identity = glm::mat4(1.0f);
			getPose(
				currentAnim,
				dancingRobot.meshes[i].skeleton, 
				currentAnim.animationTime, //Which time to sample
				currentPose, //The output vector
				identity, //Passing it an identity as it's base parent mesh
				globalInverseTransform
			);
			
			glUniformMatrix4fv(viewProjectionMatrixLocation, 1, GL_FALSE, glm::value_ptr(viewProjectionMatrix));
			glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(dancingRobot.modelMatrix));
			glUniformMatrix4fv(boneMatricesLocation, currentAnim.boneCount, GL_FALSE, glm::value_ptr(currentPose[0]));
			glBindVertexArray(dancingRobot.meshes[i].vao);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, 0);
			glUniform1i(textureLocation, 0);
			glDrawElements(GL_TRIANGLES, dancingRobot.meshes[i].indices.size(), GL_UNSIGNED_INT, 0);
		}


		GUI::Render();


		SDL_GL_SwapWindow(window);
	}

	//cleanup
	GUI::Shutdown();

	SDL_GLContext context =  SDL_GL_GetCurrentContext();
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}