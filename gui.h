#pragma once
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <SDL_events.h>
#include "MemoryMap.h"

class GUI {
public:
	static void Init(SDL_Window* window, void* context) {
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

		// Setup Platform/Renderer backends
		ImGui_ImplSDL2_InitForOpenGL(window, context);
		ImGui_ImplOpenGL3_Init();
	}

	static void ProcessEvent(const SDL_Event* event) {
		// (Where your code calls SDL_PollEvent())
		ImGui_ImplSDL2_ProcessEvent(event); // Forward your event to backend
		// (You should discard mouse/keyboard messages in your game/engine when io.WantCaptureMouse/io.WantCaptureKeyboard are set.)
	}
	
	static void StartRendering() {
		// (After event loop)
		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
		//ImGui::ShowDemoWindow(); // Show demo window! :)
		GUI::DebugWindow();
	}

	static void Render() {
		// Rendering
		// (Your code clears your framebuffer, renders your other stuff etc.)
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		// (Your code calls SDL_GL_SwapWindow() etc.)
	}

	static void Shutdown() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}

	static void DebugWindow() {
		//Creating the window flags
		ImGuiWindowFlags window_flags = 
			ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_MenuBar
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoNav
			| ImGuiWindowFlags_NoBringToFrontOnFocus;

		ImGui::Begin("Info Window", NULL, window_flags);
		ImGui::SetWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetWindowSize(ImVec2(300, 300));
		ImGui::SeparatorText("Memory Monitor");
		for (int i = 0; i < MEMORY_CHUNKS::CHUNKS; i++) {
			float percent = (float)MEMORY_CHUNKS::USED[i] / (float)MEMORY_CHUNKS::BYTES_PER_CHUNK;
			percent *= 100.0f;
			ImGui::Text("Memory Chunk %d %.2f%% (%.2f/%.2f) MB", i, percent, ((float)MEMORY_CHUNKS::USED[i]) / 1000000, ((float)MEMORY_CHUNKS::BYTES_PER_CHUNK)/1000000);
		}
		//ImGui::Text("TESTING STUFF");
		ImGui::End();
	}
};