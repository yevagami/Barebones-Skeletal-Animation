#pragma once
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <format>
#include <string>

class CSV_IO {
public:
	static void Start(const char* path) { currentStream.open(path, std::ios::out); }
	static void SetHeader(const char* header) { currentStream << header << "\n"; }
	static void Write(const char* entry) { currentStream << entry; }
	static void WriteLine(const char* entry) { currentStream << entry << "\n"; }
	static void Stop() { currentStream.close(); }
	static std::string FromVec3(glm::vec3& in) {
		std::string result;
		result = "(" + std::to_string(in.x) + " " + std::to_string(in.y) + " " + std::to_string(in.z) + ")";
		return result;
	}
	static std::string FromQuat(glm::quat& in) {
		std::string result;
		result = "(" + std::to_string(in.w) + " " + std::to_string(in.x) + " " + std::to_string(in.y) + " " + std::to_string(in.z) + ")";
		return result;
	}
	static std::string FromMat4(glm::mat4& in) {
		std::string result;
		glm::vec3 row1, row2, row3;
		row1 = glm::vec3(in[0][0], in[1][0], in[2][0]);
		row2 = glm::vec3(in[0][1], in[1][1], in[2][1]);
		row3 = glm::vec3(in[0][2], in[1][2], in[2][2]);
		result = "\"" + FromVec3(row1) + "\n";
		result += FromVec3(row2) + "\n";
		result += FromVec3(row3) + "\"";
		return result;
	}
private:
	static std::ofstream currentStream;
};
std::ofstream CSV_IO::currentStream;
