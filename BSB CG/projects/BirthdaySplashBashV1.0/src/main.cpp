/*Alexander Chow - 100749034
James Pham - 100741773
Trey Cowell - 100745472
Frederic Lai - 100748388
Anita Lim - 100754729

Birthday Splash Bash (DEMO) is a 1v1 duel between 2 players.
First player to hit their opponent 3 times is the winner!
After you fire you water gun, you have to search and walk over a water bottle to reload.

Player 1 Yellow Left: W (Forward), A (Left), S (Back), D (Right), E (Shoot).
Player 2 Pink Right: I (Forward), J (Left), K (Back) L (Right), O (Shoot).

We have been using Parsec, a screen sharing program, to play online "locally" with each other.
*/
#include <Logging.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <json.hpp>
#include <fstream>

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

#include "Graphics/IndexBuffer.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/Shader.h"
#include "Gameplay/Camera.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Behaviours/CameraControlBehaviour.h"
#include "Behaviours/FollowPathBehaviour.h"
#include "Behaviours/SimpleMoveBehaviour.h"
#include "Behaviours/PlayerMovement.h"
#include "Gameplay/Application.h"
#include "Gameplay/GameObjectTag.h"
#include "Gameplay/IBehaviour.h"
#include "Gameplay/Transform.h"
#include "Graphics/Texture2D.h"
#include "Graphics/Texture2DData.h"
#include "Utilities/InputHelpers.h"
#include "Utilities/MeshBuilder.h"
#include "Utilities/MeshFactory.h"
#include "Utilities/NotObjLoader.h"
#include "Utilities/ObjLoader.h"
#include "Utilities/VertexTypes.h"
#include "Gameplay/Scene.h"
#include "Gameplay/ShaderMaterial.h"
#include "Gameplay/RendererComponent.h"
#include "Gameplay/Timing.h"
#include "Graphics/TextureCubeMap.h"
#include "Graphics/TextureCubeMapData.h"
#include "Utilities/Util.h"

#define LOG_GL_NOTIFICATIONS
#define NUM_TREES 25
#define PLANE_X 19.0f
#define PLANE_Y 19.0f
#define DNS_X 3.0f
#define DNS_Y 3.0f
#define NUM_HITBOXES_TEST 2

/*
	Handles debug messages from OpenGL
	https://www.khronos.org/opengl/wiki/Debug_Output#Message_Components
	@param source    Which part of OpenGL dispatched the message
	@param type      The type of message (ex: error, performance issues, deprecated behavior)
	@param id        The ID of the error or message (to distinguish between different types of errors, like nullref or index out of range)
	@param severity  The severity of the message (from High to Notification)
	@param length    The length of the message
	@param message   The human readable message from OpenGL
	@param userParam The pointer we set with glDebugMessageCallback (should be the game pointer)
*/
void GlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	std::string sourceTxt;
	switch (source) {
	case GL_DEBUG_SOURCE_API: sourceTxt = "DEBUG"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceTxt = "WINDOW"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceTxt = "SHADER"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY: sourceTxt = "THIRD PARTY"; break;
	case GL_DEBUG_SOURCE_APPLICATION: sourceTxt = "APP"; break;
	case GL_DEBUG_SOURCE_OTHER: default: sourceTxt = "OTHER"; break;
	}
	switch (severity) {
	case GL_DEBUG_SEVERITY_LOW:          LOG_INFO("[{}] {}", sourceTxt, message); break;
	case GL_DEBUG_SEVERITY_MEDIUM:       LOG_WARN("[{}] {}", sourceTxt, message); break;
	case GL_DEBUG_SEVERITY_HIGH:         LOG_ERROR("[{}] {}", sourceTxt, message); break;
		#ifdef LOG_GL_NOTIFICATIONS
	case GL_DEBUG_SEVERITY_NOTIFICATION: LOG_INFO("[{}] {}", sourceTxt, message); break;
		#endif
	default: break;
	}
}

GLFWwindow* window;

void GlfwWindowResizedCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	Application::Instance().ActiveScene->Registry().view<Camera>().each([=](Camera & cam) {
		cam.ResizeWindow(width, height);
	});
}

bool InitGLFW() {
	if (glfwInit() == GLFW_FALSE) {
		LOG_ERROR("Failed to initialize GLFW");
		return false;
	}

#ifdef _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif
	
	//Create a new GLFW window
	window = glfwCreateWindow(800, 800, "Birthday Splash Bash", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Set our window resized callback
	glfwSetWindowSizeCallback(window, GlfwWindowResizedCallback);

	// Store the window in the application singleton
	Application::Instance().Window = window;

	return true;
}

bool InitGLAD() {
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
		LOG_ERROR("Failed to initialize Glad");
		return false;
	}
	return true;
}

void InitImGui() {
	// Creates a new ImGUI context
	ImGui::CreateContext();
	// Gets our ImGUI input/output 
	ImGuiIO& io = ImGui::GetIO();
	// Enable keyboard navigation
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	// Allow docking to our window
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// Allow multiple viewports (so we can drag ImGui off our window)
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	// Allow our viewports to use transparent backbuffers
	io.ConfigFlags |= ImGuiConfigFlags_TransparentBackbuffers;

	// Set up the ImGui implementation for OpenGL
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 410");

	// Dark mode FTW
	ImGui::StyleColorsDark();

	// Get our imgui style
	ImGuiStyle& style = ImGui::GetStyle();
	//style.Alpha = 1.0f;
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 0.8f;
	}
}

void ShutdownImGui()
{
	// Cleanup the ImGui implementation
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	// Destroy our ImGui context
	ImGui::DestroyContext();
}

std::vector<std::function<void()>> imGuiCallbacks;
void RenderImGui() {
	// Implementation new frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	// ImGui context new frame
	ImGui::NewFrame();

	if (ImGui::Begin("Debug")) {
		// Render our GUI stuff
		for (auto& func : imGuiCallbacks) {
			func();
		}
		ImGui::End();
	}
	
	// Make sure ImGui knows how big our window is
	ImGuiIO& io = ImGui::GetIO();
	int width{ 0 }, height{ 0 };
	glfwGetWindowSize(window, &width, &height);
	io.DisplaySize = ImVec2((float)width, (float)height);

	// Render all of our ImGui elements
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// If we have multiple viewports enabled (can drag into a new window)
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		// Update the windows that ImGui is using
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		// Restore our gl context
		glfwMakeContextCurrent(window);
	}
}

// Borrowed collision from https://learnopengl.com/In-Practice/2D-Game/Collisions/Collision-detection AABB collision
bool Collision(Transform& hitbox1, Transform& hitbox2)
{
	bool colX = hitbox1.GetLocalPosition().x + hitbox1.GetLocalScale().x >= hitbox2.GetLocalPosition().x
		&& hitbox2.GetLocalPosition().x + hitbox2.GetLocalScale().x >= hitbox1.GetLocalPosition().x;

	bool colY = hitbox1.GetLocalPosition().y + hitbox1.GetLocalScale().y >= hitbox2.GetLocalPosition().y
		&& hitbox2.GetLocalPosition().y + hitbox2.GetLocalScale().y >= hitbox1.GetLocalPosition().y;
	return colX && colY;
}

void RenderVAO(
	const Shader::sptr& shader,
	const VertexArrayObject::sptr& vao,
	const glm::mat4& viewProjection,
	const Transform& transform)
{
	shader->SetUniformMatrix("u_ModelViewProjection", viewProjection * transform.WorldTransform());
	shader->SetUniformMatrix("u_Model", transform.WorldTransform()); 
	shader->SetUniformMatrix("u_NormalMatrix", transform.WorldNormalMatrix());
	vao->Render();
}

void SetupShaderForFrame(const Shader::sptr& shader, const glm::mat4& view, const glm::mat4& projection) {
	shader->Bind();
	// These are the uniforms that update only once per frame
	shader->SetUniformMatrix("u_View", view);
	shader->SetUniformMatrix("u_ViewProjection", projection * view);
	shader->SetUniformMatrix("u_SkyboxMatrix", projection * glm::mat4(glm::mat3(view)));
	glm::vec3 camPos = glm::inverse(view) * glm::vec4(0,0,0,1);
	shader->SetUniform("u_CamPos", camPos);
}

int main() {
	Logger::Init(); // We'll borrow the logger from the toolkit, but we need to initialize it

	//Initialize GLFW
	if (!InitGLFW())
		return 1;

	//Initialize GLAD
	if (!InitGLAD())
		return 1;

	int frameIx = 0;
	float fpsBuffer[128];
	float minFps, maxFps, avgFps;
	int selectedVao = 0; // select cube by default
	std::vector<GameObject> controllables;

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(GlDebugMessage, nullptr);

	// Enable texturing
	glEnable(GL_TEXTURE_2D);

	// Push another scope so most memory should be freed *before* we exit the app
	{
		#pragma region Shader and ImGui

		// Load our shaders
		Shader::sptr shader = Shader::Create();
		shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		shader->LoadShaderPartFromFile("shaders/frag_blinn_phong_textured.glsl", GL_FRAGMENT_SHADER);
		shader->Link();

		// Load a second material for our reflective material!
		Shader::sptr reflectiveShader = Shader::Create();
		reflectiveShader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		reflectiveShader->LoadShaderPartFromFile("shaders/frag_reflection.frag.glsl", GL_FRAGMENT_SHADER);
		reflectiveShader->Link();

		Shader::sptr reflective = Shader::Create();
		reflective->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		reflective->LoadShaderPartFromFile("shaders/frag_blinn_phong_reflection.glsl", GL_FRAGMENT_SHADER);
		reflective->Link();

		glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 10.0f);
		glm::vec3 lightCol = glm::vec3(0.9f, 0.85f, 0.5f);
		float     lightAmbientPow = 2.1f;
		float     lightSpecularPow = 1.0f;
		glm::vec3 ambientCol = glm::vec3(1.0f);
		float     ambientPow = 0.1f;
		float     lightLinearFalloff = 0.009;
		float     lightQuadraticFalloff = 0.032f;
		//variables for turning lighting off and on
		int		  lightoff = 0;
		int		  ambientonly = 0;
		int		  specularonly = 0;
		int		  ambientandspecular = 0;
		int		  ambientspeculartoon = 0;

		// These are our application / scene level uniforms that don't necessarily update
		// every frame
		shader->SetUniform("u_LightPos", lightPos);
		shader->SetUniform("u_LightCol", lightCol);
		shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
		shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
		shader->SetUniform("u_AmbientCol", ambientCol);
		shader->SetUniform("u_AmbientStrength", ambientPow);
		shader->SetUniform("u_LightAttenuationConstant", 1.0f);
		shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
		shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);
		shader->SetUniform("u_lightoff", lightoff);
		shader->SetUniform("u_ambient", ambientonly);
		shader->SetUniform("u_specular", specularonly);
		shader->SetUniform("u_ambientspecular", ambientandspecular);
		shader->SetUniform("u_ambientspeculartoon", ambientspeculartoon);

		// We'll add some ImGui controls to control our shader
		imGuiCallbacks.push_back([&]() {
			if (ImGui::CollapsingHeader("Scene Level Lighting Settings"))
			{
				if (ImGui::ColorPicker3("Ambient Color", glm::value_ptr(ambientCol))) {
					shader->SetUniform("u_AmbientCol", ambientCol);
				}
				if (ImGui::SliderFloat("Fixed Ambient Power", &ambientPow, 0.01f, 1.0f)) {
					shader->SetUniform("u_AmbientStrength", ambientPow);
				}
			}
			if (ImGui::CollapsingHeader("Light Level Lighting Settings"))
			{
				if (ImGui::DragFloat3("Light Pos", glm::value_ptr(lightPos), 0.01f, -10.0f, 10.0f)) {
					shader->SetUniform("u_LightPos", lightPos);
				}
				if (ImGui::ColorPicker3("Light Col", glm::value_ptr(lightCol))) {
					shader->SetUniform("u_LightCol", lightCol);
				}
				if (ImGui::SliderFloat("Light Ambient Power", &lightAmbientPow, 0.0f, 1.0f)) {
					shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
				}
				if (ImGui::SliderFloat("Light Specular Power", &lightSpecularPow, 0.0f, 1.0f)) {
					shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
				}
				if (ImGui::DragFloat("Light Linear Falloff", &lightLinearFalloff, 0.01f, 0.0f, 1.0f)) {
					shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
				}
				if (ImGui::DragFloat("Light Quadratic Falloff", &lightQuadraticFalloff, 0.01f, 0.0f, 1.0f)) {
					shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);
				}
			}

			/*Toggle buttons
			No lighting
			Ambient only
			specular only
			Ambient and specular
			Ambient, Specular, and Toon Shader*/
			if (ImGui::CollapsingHeader("Toggle buttons")) {
				if (ImGui::Button("No Lighting")) {
					shader->SetUniform("u_lightoff", lightoff = 1);
					shader->SetUniform("u_ambient", ambientonly = 0);
					shader->SetUniform("u_specular", specularonly = 0);
					shader->SetUniform("u_ambientspecular", ambientandspecular = 0);
					shader->SetUniform("u_ambientspeculartoon", ambientspeculartoon = 0);
				}

				if (ImGui::Button("Ambient only")) {
					shader->SetUniform("u_lightoff", lightoff = 0);
					shader->SetUniform("u_ambient", ambientonly = 1);
					shader->SetUniform("u_specular", specularonly = 0);
					shader->SetUniform("u_ambientspecular", ambientandspecular = 0);
					shader->SetUniform("u_ambientspeculartoon", ambientspeculartoon = 0);
				}

				if (ImGui::Button("specular only")) {
					shader->SetUniform("u_lightoff", lightoff = 0);
					shader->SetUniform("u_ambient", ambientonly = 0);
					shader->SetUniform("u_specular", specularonly = 1);
					shader->SetUniform("u_ambientspecular", ambientandspecular = 0);
					shader->SetUniform("u_ambientspeculartoon", ambientspeculartoon = 0);
				}

				if (ImGui::Button("Ambient and Specular")) {
					shader->SetUniform("u_lightoff", lightoff = 0);
					shader->SetUniform("u_ambient", ambientonly = 0);
					shader->SetUniform("u_specular", specularonly = 0);
					shader->SetUniform("u_ambientspecular", ambientandspecular = 1);
					shader->SetUniform("u_ambientspeculartoon", ambientspeculartoon = 0);
				}

				if (ImGui::Button("Ambient, Specular, and Toon Shading")) {
					shader->SetUniform("u_lightoff", lightoff = 0);
					shader->SetUniform("u_ambient", ambientonly = 0);
					shader->SetUniform("u_specular", specularonly = 0);
					shader->SetUniform("u_ambientspecular", ambientandspecular = 0);
					shader->SetUniform("u_ambientspeculartoon", ambientspeculartoon = 1);
				}
			}

			auto name = controllables[selectedVao].get<GameObjectTag>().Name;
			ImGui::Text(name.c_str());
			auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
			ImGui::Checkbox("Relative Rotation", &behaviour->Relative);

			ImGui::Text("Q/E -> Yaw\nLeft/Right -> Roll\nUp/Down -> Pitch\nY -> Toggle Mode");

			minFps = FLT_MAX;
			maxFps = 0;
			avgFps = 0;
			for (int ix = 0; ix < 128; ix++) {
				if (fpsBuffer[ix] < minFps) { minFps = fpsBuffer[ix]; }
				if (fpsBuffer[ix] > maxFps) { maxFps = fpsBuffer[ix]; }
				avgFps += fpsBuffer[ix];
			}
			ImGui::PlotLines("FPS", fpsBuffer, 128);
			ImGui::Text("MIN: %f MAX: %f AVG: %f", minFps, maxFps, avgFps / 128.0f);
			});

		#pragma endregion Shader and ImGui

		// GL states
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL); // New 

		#pragma region TEXTURE LOADING

		#pragma region testing scene difuses
		// Load some textures from files
		Texture2D::sptr diffuse = Texture2D::LoadFromFile("images/TestScene/Stone_001_Diffuse.png");
		Texture2D::sptr diffuseGround = Texture2D::LoadFromFile("images/TestScene/grass.jpg");
		Texture2D::sptr diffuseDunce = Texture2D::LoadFromFile("images/TestScene/Dunce.png");
		Texture2D::sptr diffuseDuncet = Texture2D::LoadFromFile("images/TestScene/Duncet.png");
		Texture2D::sptr diffuseSlide = Texture2D::LoadFromFile("images/TestScene/Slide.png");
		Texture2D::sptr diffuseSwing = Texture2D::LoadFromFile("images/TestScene/Swing.png");
		Texture2D::sptr diffuseTable = Texture2D::LoadFromFile("images//TestScene/Table.png");
		Texture2D::sptr diffuseTreeBig = Texture2D::LoadFromFile("images/TestScene/TreeBig.png");
		Texture2D::sptr diffuseRedBalloon = Texture2D::LoadFromFile("images/TestScene/BalloonRed.png");
		Texture2D::sptr diffuseYellowBalloon = Texture2D::LoadFromFile("images/TestScene/BalloonYellow.png");
		Texture2D::sptr diffuse2 = Texture2D::LoadFromFile("images/TestScene/box.bmp");
		Texture2D::sptr specular = Texture2D::LoadFromFile("images/TestScene/Stone_001_Specular.png");
		Texture2D::sptr reflectivity = Texture2D::LoadFromFile("images/TestScene/box-reflections.bmp");
		#pragma endregion testing scene difuses

		#pragma region Arena1 diffuses
		Texture2D::sptr diffuseTrees = Texture2D::LoadFromFile("images/Arena1/Trees.png");
		Texture2D::sptr diffuseFlowers = Texture2D::LoadFromFile("images/Arena1/Flower.png");
		Texture2D::sptr diffuseGroundArena = Texture2D::LoadFromFile("images/Arena1/Ground.png");
		Texture2D::sptr diffuseHedge = Texture2D::LoadFromFile("images/Arena1/Hedge.png");
		Texture2D::sptr diffuseBalloons = Texture2D::LoadFromFile("images/Arena1/Ballons.png");
		Texture2D::sptr diffuseDunceArena = Texture2D::LoadFromFile("images/Arena1/Dunce.png");
		Texture2D::sptr diffuseDuncetArena = Texture2D::LoadFromFile("images/Arena1/Duncet.png");
		Texture2D::sptr diffusered = Texture2D::LoadFromFile("images/Arena1/red.png");
		Texture2D::sptr diffuseyellow = Texture2D::LoadFromFile("images/Arena1/yellow.png");
		Texture2D::sptr diffusepink = Texture2D::LoadFromFile("images/Arena1/pink.png");
		Texture2D::sptr diffusemonkeybar = Texture2D::LoadFromFile("images/Arena1/MonkeyBar.png");
		Texture2D::sptr diffusecake = Texture2D::LoadFromFile("images/Arena1/SliceOfCake.png");
		Texture2D::sptr diffusesandbox = Texture2D::LoadFromFile("images/Arena1/SandBox.png");
		Texture2D::sptr diffuseroundabout = Texture2D::LoadFromFile("images/Arena1/RoundAbout.png");
		Texture2D::sptr diffusepinwheel = Texture2D::LoadFromFile("images/Arena1/Pinwheel.png");
		#pragma endregion Arena1 diffuses

		// Load the cube map
		//TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/sample.jpg");
		TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/ocean.jpg"); 

		// Creating an empty texture
		Texture2DDescription desc = Texture2DDescription();  
		desc.Width = 1;
		desc.Height = 1;
		desc.Format = InternalFormat::RGB8;
		Texture2D::sptr texture2 = Texture2D::Create(desc);
		// Clear it with a white colour
		texture2->Clear();

		#pragma endregion TEXTURE LOADING

		#pragma region Scene Generation
		
		// We need to tell our scene system what extra component types we want to support
		GameScene::RegisterComponentType<RendererComponent>();
		GameScene::RegisterComponentType<BehaviourBinding>();
		GameScene::RegisterComponentType<Camera>();

		// Create scenes, and set menu to be the active scene in the application
		GameScene::sptr scene = GameScene::Create("test");
		GameScene::sptr Arena1 = GameScene::Create("Arena1");
		GameScene::sptr Arena2 = GameScene::Create("Arena2(beta)");//Might not use this
		GameScene::sptr Menu = GameScene::Create("Menu");
		GameScene::sptr Instructions = GameScene::Create("Instructions");
		GameScene::sptr Pause = GameScene::Create("Pause");
		GameScene::sptr WinandLose = GameScene::Create("Win/Lose");
		Application::Instance().ActiveScene = Menu;

		// We can create a group ahead of time to make iterating on the group faster
		entt::basic_group<entt::entity, entt::exclude_t<>, entt::get_t<Transform>, RendererComponent> renderGroup =
			scene->Registry().group<RendererComponent>(entt::get_t<Transform>());
		
		entt::basic_group<entt::entity, entt::exclude_t<>, entt::get_t<Transform>, RendererComponent> renderGroupArena =
			Arena1->Registry().group<RendererComponent>(entt::get_t<Transform>());
		
		entt::basic_group<entt::entity, entt::exclude_t<>, entt::get_t<Transform>, RendererComponent> renderGroupPause =
			Pause->Registry().group<RendererComponent>(entt::get_t<Transform>());
		
		entt::basic_group<entt::entity, entt::exclude_t<>, entt::get_t<Transform>, RendererComponent> renderGroupMenu =
			Menu->Registry().group<RendererComponent>(entt::get_t<Transform>());

		#pragma endregion Scene Generation

		// Create materials and set some properties for them
		ShaderMaterial::sptr materialGround = ShaderMaterial::Create();  
		materialGround->Shader = shader;
		materialGround->Set("s_Diffuse", diffuseGround);
		materialGround->Set("s_Diffuse2", diffuse2);
		materialGround->Set("s_Specular", specular);
		materialGround->Set("u_Shininess", 8.0f);
		materialGround->Set("u_TextureMix", 0.0f); 
		
		ShaderMaterial::sptr materialDunce = ShaderMaterial::Create();  
		materialDunce->Shader = shader;
		materialDunce->Set("s_Diffuse", diffuseDunce);
		materialDunce->Set("s_Diffuse2", diffuse2);
		materialDunce->Set("s_Specular", specular);
		materialDunce->Set("u_Shininess", 8.0f);
		materialDunce->Set("u_TextureMix", 0.0f); 
		
		ShaderMaterial::sptr materialDuncet = ShaderMaterial::Create();  
		materialDuncet->Shader = shader;
		materialDuncet->Set("s_Diffuse", diffuseDuncet);
		materialDuncet->Set("s_Diffuse2", diffuse2);
		materialDuncet->Set("s_Specular", specular);
		materialDuncet->Set("u_Shininess", 8.0f);
		materialDuncet->Set("u_TextureMix", 0.0f); 

		ShaderMaterial::sptr materialSlide = ShaderMaterial::Create();  
		materialSlide->Shader = shader;
		materialSlide->Set("s_Diffuse", diffuseSlide);
		materialSlide->Set("s_Diffuse2", diffuse2);
		materialSlide->Set("s_Specular", specular);
		materialSlide->Set("u_Shininess", 8.0f);
		materialSlide->Set("u_TextureMix", 0.0f);
		
		ShaderMaterial::sptr materialSwing = ShaderMaterial::Create();  
		materialSwing->Shader = shader;
		materialSwing->Set("s_Diffuse", diffuseSwing);
		materialSwing->Set("s_Diffuse2", diffuse2);
		materialSwing->Set("s_Specular", specular);
		materialSwing->Set("u_Shininess", 8.0f);
		materialSwing->Set("u_TextureMix", 0.0f);
		
		ShaderMaterial::sptr materialMonkeyBar = ShaderMaterial::Create();  
		materialMonkeyBar->Shader = shader;
		materialMonkeyBar->Set("s_Diffuse", diffusemonkeybar);
		materialMonkeyBar->Set("s_Diffuse2", diffuse2);
		materialMonkeyBar->Set("s_Specular", specular);
		materialMonkeyBar->Set("u_Shininess", 8.0f);
		materialMonkeyBar->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr materialSliceOfCake = ShaderMaterial::Create();
		materialSliceOfCake->Shader = shader;
		materialSliceOfCake->Set("s_Diffuse", diffusecake);
		materialSliceOfCake->Set("s_Diffuse2", diffuse2);
		materialSliceOfCake->Set("s_Specular", specular);
		materialSliceOfCake->Set("u_Shininess", 8.0f);
		materialSliceOfCake->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr materialSandBox = ShaderMaterial::Create();
		materialSandBox->Shader = shader;
		materialSandBox->Set("s_Diffuse", diffusesandbox);
		materialSandBox->Set("s_Diffuse2", diffuse2);
		materialSandBox->Set("s_Specular", specular);
		materialSandBox->Set("u_Shininess", 8.0f);
		materialSandBox->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr materialRA = ShaderMaterial::Create();
		materialRA->Shader = shader;
		materialRA->Set("s_Diffuse", diffuseroundabout);
		materialRA->Set("s_Diffuse2", diffuse2);
		materialRA->Set("s_Specular", specular);
		materialRA->Set("u_Shininess", 8.0f);
		materialRA->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr materialPinwheel = ShaderMaterial::Create();
		materialPinwheel->Shader = shader;
		materialPinwheel->Set("s_Diffuse", diffusepinwheel);
		materialPinwheel->Set("s_Diffuse2", diffuse2);
		materialPinwheel->Set("s_Specular", specular);
		materialPinwheel->Set("u_Shininess", 8.0f);
		materialPinwheel->Set("u_TextureMix", 0.0f);
		
		ShaderMaterial::sptr materialTable = ShaderMaterial::Create();  
		materialTable->Shader = shader;
		materialTable->Set("s_Diffuse", diffuseTable);
		materialTable->Set("s_Diffuse2", diffuse2);
		materialTable->Set("s_Specular", specular);
		materialTable->Set("u_Shininess", 8.0f);
		materialTable->Set("u_TextureMix", 0.0f);
		
		ShaderMaterial::sptr materialTreeBig = ShaderMaterial::Create();  
		materialTreeBig->Shader = shader;
		materialTreeBig->Set("s_Diffuse", diffuseTreeBig);
		materialTreeBig->Set("s_Diffuse2", diffuse2);
		materialTreeBig->Set("s_Specular", specular);
		materialTreeBig->Set("u_Shininess", 8.0f);
		materialTreeBig->Set("u_TextureMix", 0.0f);
		
		ShaderMaterial::sptr materialredballoon = ShaderMaterial::Create();  
		materialredballoon->Shader = shader;
		materialredballoon->Set("s_Diffuse", diffuseRedBalloon);
		materialredballoon->Set("s_Diffuse2", diffuse2);
		materialredballoon->Set("s_Specular", specular);
		materialredballoon->Set("u_Shininess", 8.0f);
		materialredballoon->Set("u_TextureMix", 0.0f);
		
		ShaderMaterial::sptr materialyellowballoon = ShaderMaterial::Create();  
		materialyellowballoon->Shader = shader;
		materialyellowballoon->Set("s_Diffuse", diffuseYellowBalloon);
		materialyellowballoon->Set("s_Diffuse2", diffuse2);
		materialyellowballoon->Set("s_Specular", specular);
		materialyellowballoon->Set("u_Shininess", 8.0f);
		materialyellowballoon->Set("u_TextureMix", 0.0f);
		
		ShaderMaterial::sptr materialtrees = ShaderMaterial::Create();  
		materialtrees->Shader = shader;
		materialtrees->Set("s_Diffuse", diffuseTrees);
		materialtrees->Set("s_Diffuse2", diffuse2);
		materialtrees->Set("s_Specular", specular);
		materialtrees->Set("u_Shininess", 8.0f);
		materialtrees->Set("u_TextureMix", 0.0f);
		
		ShaderMaterial::sptr materialflowers = ShaderMaterial::Create();  
		materialflowers->Shader = shader;
		materialflowers->Set("s_Diffuse", diffuseFlowers);
		materialflowers->Set("s_Diffuse2", diffuse2);
		materialflowers->Set("s_Specular", specular);
		materialflowers->Set("u_Shininess", 8.0f);
		materialflowers->Set("u_TextureMix", 0.0f);
		
		ShaderMaterial::sptr materialGroundArena = ShaderMaterial::Create();  
		materialGroundArena->Shader = shader;
		materialGroundArena->Set("s_Diffuse", diffuseGroundArena);
		materialGroundArena->Set("s_Diffuse2", diffuse2);
		materialGroundArena->Set("s_Specular", specular);
		materialGroundArena->Set("u_Shininess", 8.0f);
		materialGroundArena->Set("u_TextureMix", 0.0f);
		
		ShaderMaterial::sptr materialHedge = ShaderMaterial::Create();  
		materialHedge->Shader = shader;
		materialHedge->Set("s_Diffuse", diffuseHedge);
		materialHedge->Set("s_Diffuse2", diffuse2);
		materialHedge->Set("s_Specular", specular);
		materialHedge->Set("u_Shininess", 8.0f);
		materialHedge->Set("u_TextureMix", 0.0f);
		
		ShaderMaterial::sptr materialBalloons = ShaderMaterial::Create();  
		materialBalloons->Shader = shader;
		materialBalloons->Set("s_Diffuse", diffuseBalloons);
		materialBalloons->Set("s_Diffuse2", diffuse2);
		materialBalloons->Set("s_Specular", specular);
		materialBalloons->Set("u_Shininess", 8.0f);
		materialBalloons->Set("u_TextureMix", 0.0f);
		
		ShaderMaterial::sptr materialDunceArena = ShaderMaterial::Create();  
		materialDunceArena->Shader = shader;
		materialDunceArena->Set("s_Diffuse", diffuseDunceArena);
		materialDunceArena->Set("s_Diffuse2", diffuse2);
		materialDunceArena->Set("s_Specular", specular);
		materialDunceArena->Set("u_Shininess", 8.0f);
		materialDunceArena->Set("u_TextureMix", 0.0f);
		
		ShaderMaterial::sptr materialDuncetArena = ShaderMaterial::Create();  
		materialDuncetArena->Shader = shader;
		materialDuncetArena->Set("s_Diffuse", diffuseDuncetArena);
		materialDuncetArena->Set("s_Diffuse2", diffuse2);
		materialDuncetArena->Set("s_Specular", specular);
		materialDuncetArena->Set("u_Shininess", 8.0f);
		materialDuncetArena->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr materialBottleyellow = ShaderMaterial::Create();  
		materialBottleyellow->Shader = shader;
		materialBottleyellow->Set("s_Diffuse", diffuseyellow);
		materialBottleyellow->Set("s_Diffuse2", diffuse2);
		materialBottleyellow->Set("s_Specular", specular);
		materialBottleyellow->Set("u_Shininess", 8.0f);
		materialBottleyellow->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr materialBottlepink = ShaderMaterial::Create();  
		materialBottlepink->Shader = shader;
		materialBottlepink->Set("s_Diffuse", diffusepink);
		materialBottlepink->Set("s_Diffuse2", diffuse2);
		materialBottlepink->Set("s_Specular", specular);
		materialBottlepink->Set("u_Shininess", 8.0f);
		materialBottlepink->Set("u_TextureMix", 0.0f);

		// 
		ShaderMaterial::sptr material1 = ShaderMaterial::Create();
		material1->Shader = reflective;
		material1->Set("s_Diffuse", diffuse);
		material1->Set("s_Diffuse2", diffuse2);
		material1->Set("s_Specular", specular);
		material1->Set("s_Reflectivity", reflectivity);
		material1->Set("s_Environment", environmentMap);
		material1->Set("u_LightPos", lightPos);
		material1->Set("u_LightCol", lightCol);
		material1->Set("u_AmbientLightStrength", lightAmbientPow);
		material1->Set("u_SpecularLightStrength", lightSpecularPow);
		material1->Set("u_AmbientCol", ambientCol);
		material1->Set("u_AmbientStrength", ambientPow);
		material1->Set("u_LightAttenuationConstant", 1.0f);
		material1->Set("u_LightAttenuationLinear", lightLinearFalloff);
		material1->Set("u_LightAttenuationQuadratic", lightQuadraticFalloff);
		material1->Set("u_Shininess", 8.0f);
		material1->Set("u_TextureMix", 0.5f);
		material1->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));

		ShaderMaterial::sptr reflectiveMat = ShaderMaterial::Create();
		reflectiveMat->Shader = reflectiveShader;
		reflectiveMat->Set("s_Environment", environmentMap);
		reflectiveMat->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));

		// Create an object to be our camera
		GameObject cameraObject = scene->CreateEntity("Camera");
		{
			cameraObject.get<Transform>().SetLocalPosition(0, 1, 17).LookAt(glm::vec3(0, 0, 0));

			// We'll make our camera a component of the camera object
			Camera& camera = cameraObject.emplace<Camera>();// Camera::Create();
			camera.SetPosition(glm::vec3(0, 3, 3));
			camera.SetUp(glm::vec3(0, 0, 1));
			camera.LookAt(glm::vec3(0));
			camera.SetFovDegrees(90.0f); // Set an initial FOV
			camera.SetOrthoHeight(3.0f);
			BehaviourBinding::Bind<CameraControlBehaviour>(cameraObject);
		}

		#pragma region Test(scene) Objects

		GameObject objGround = scene->CreateEntity("Ground"); 
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TestScene/Ground.obj");
			objGround.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialGround);
			objGround.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			objGround.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			objGround.get<Transform>().SetLocalScale(0.5f, 0.25f, 0.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(objGround);
		}

		GameObject objDunce = scene->CreateEntity("Dunce");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TestScene/Dunce.obj");
			objDunce.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialDunce);
			objDunce.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.9f);
			objDunce.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(objDunce);
		}

		GameObject objDuncet = scene->CreateEntity("Duncet");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TestScene/Duncet.obj");
			objDuncet.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialDuncet);
			objDuncet.get<Transform>().SetLocalPosition(2.0f, 0.0f, 0.8f);
			objDuncet.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(objDuncet);
		}


		GameObject objSlide = scene->CreateEntity("Slide");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TestScene/Slide.obj");
			objSlide.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialSlide);
			objSlide.get<Transform>().SetLocalPosition(0.0f, 5.0f, 3.0f);
			objSlide.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			objSlide.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(objSlide);
		}
		
		GameObject objRedBalloon = scene->CreateEntity("Redballoon");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TestScene/Balloon.obj");
			objRedBalloon.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialredballoon);
			objRedBalloon.get<Transform>().SetLocalPosition(2.5f, -10.0f, 3.0f);
			objRedBalloon.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			objRedBalloon.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(objRedBalloon);

			// Bind returns a smart pointer to the behaviour that was added
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(objRedBalloon);
			// Set up a path for the object to follow
			pathing->Points.push_back({ -2.5f, -10.0f, 3.0f });
			pathing->Points.push_back({ 2.5f, -10.0f, 3.0f });
			pathing->Points.push_back({ 2.5f, -5.0f, 3.0f });
			pathing->Points.push_back({ -2.5f, -5.0f, 3.0f });
			pathing->Speed = 2.0f;
		}
		
		GameObject objYellowBalloon = scene->CreateEntity("Yellowballoon");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TestScene/Balloon.obj");
			objYellowBalloon.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialyellowballoon);
			objYellowBalloon.get<Transform>().SetLocalPosition(-2.5f, -10.0f, 3.0f);
			objYellowBalloon.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			objYellowBalloon.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(objYellowBalloon);

			// Bind returns a smart pointer to the behaviour that was added
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(objYellowBalloon);
			// Set up a path for the object to follow
			pathing->Points.push_back({ 2.5f, -10.0f, 3.0f });
			pathing->Points.push_back({ -2.5f, -10.0f, 3.0f });
			pathing->Points.push_back({ -2.5f,  -5.0f, 3.0f });
			pathing->Points.push_back({ 2.5f,  -5.0f, 3.0f });
			pathing->Speed = 2.0f;
		}
		
		//Taken from week 3 tutorial because I wanted random trees from our game
		std::vector<GameObject> randomTrees;
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TestScene/TreeBig.obj");
			for (int i = 0; i < NUM_TREES / 2; i++)
			{
				randomTrees.push_back(scene->CreateEntity("simplePine" + (std::to_string(i + 1))));
				randomTrees[i].emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialTreeBig);
				//Randomly places
				randomTrees[i].get<Transform>().SetLocalPosition(glm::vec3(Util::GetRandomNumberBetween(glm::vec2(-PLANE_X, -PLANE_Y), glm::vec2(PLANE_X, PLANE_Y), glm::vec2(-DNS_X, -DNS_Y), glm::vec2(DNS_X, DNS_Y)), 6.0f));
				randomTrees[i].get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
				randomTrees[i].get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			}
		}

		GameObject objSwing = scene->CreateEntity("Swing");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TestScene/Swing.obj");
			objSwing.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialSwing);
			objSwing.get<Transform>().SetLocalPosition(-5.0f, 0.0f, 3.5f);
			objSwing.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			objSwing.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(objSwing);
		}

		GameObject objTable = scene->CreateEntity("table");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TestScene/Table.obj");
			objTable.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialTable);
			objTable.get<Transform>().SetLocalPosition(5.0f, 0.0f, 1.25f);
			objTable.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			objTable.get<Transform>().SetLocalScale(0.35f, 0.35f, 0.35f);
		}
		
		//HitBoxes generated using a for loop then each one is given a position
		std::vector<GameObject> Hitboxes;
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TestScene/HitBox.obj");
			for (int i = 0; i < NUM_HITBOXES_TEST; i++)//NUM_HITBOXES_TEST is located at the top of the code
			{
				Hitboxes.push_back(scene->CreateEntity("Hitbox" + (std::to_string(i + 1))));
				Hitboxes[i].emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialTreeBig);//Material does not matter just invisable hitboxes
			}

			Hitboxes[0].get<Transform>().SetLocalPosition(4.0f, 4.0f, 2.0f);
			Hitboxes[1].get<Transform>().SetLocalPosition(-4.0f, -4.0f, 2.0f);
		}

		#pragma endregion Test(scene) Objects

		#pragma region Arena1 Objects

		GameObject objDunceArena = Arena1->CreateEntity("Dunce");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TestScene/Dunce.obj");
			objDunceArena.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialDunceArena);
			objDunceArena.get<Transform>().SetLocalPosition(8.0f, 6.0f, 0.0f);
			objDunceArena.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
			objDunceArena.get<Transform>().SetLocalScale(1.0f, 1.0f, 1.0f);
		}
		
		GameObject objDuncetArena = Arena1->CreateEntity("Duncet");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TestScene/Duncet.obj");
			objDuncetArena.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialDuncetArena);
			objDuncetArena.get<Transform>().SetLocalPosition(-8.0f, 6.0f, 0.0f);
			objDuncetArena.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
			objDuncetArena.get<Transform>().SetLocalScale(1.0f, 1.0f, 1.0f);
		}
		
		GameObject objSlideArena = Arena1->CreateEntity("slide");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TestScene/Slide.obj");
			objSlideArena.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialSlide);
			objSlideArena.get<Transform>().SetLocalPosition(-2.0f, -2.0f, 2.0f);
			objSlideArena.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
			objSlideArena.get<Transform>().SetLocalScale(0.25f, 0.25f, 0.25f);
		}
		
		GameObject objSwingArena = Arena1->CreateEntity("swing");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TestScene/swing.obj");
			objSwingArena.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialSwing);
			objSwingArena.get<Transform>().SetLocalPosition(-4.0f, 2.0f, 2.0f);
			objSwingArena.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
			objSwingArena.get<Transform>().SetLocalScale(0.25f, 0.25f, 0.25f);
		}

		GameObject objMonkeyBarArena = Arena1->CreateEntity("monkeybar");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Arena1/MonkeyBar.obj");
			objMonkeyBarArena.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialMonkeyBar);
			objMonkeyBarArena.get<Transform>().SetLocalPosition(2.0f, 2.0f, 2.0f);
			objMonkeyBarArena.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
			objMonkeyBarArena.get<Transform>().SetLocalScale(0.25f, 0.25f, 0.25f);
		}

		GameObject objcakeArena = Arena1->CreateEntity("cake");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Arena1/SliceofCake.obj");
			objcakeArena.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialSliceOfCake);
			objcakeArena.get<Transform>().SetLocalPosition(6.0f, -2.0f, 0.0f);
			objcakeArena.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
			objcakeArena.get<Transform>().SetLocalScale(0.25f, 0.25f, 0.25f);
		}

		GameObject objSandBoxArena = Arena1->CreateEntity("sandBox");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Arena1/SandBox.obj");
			objSandBoxArena.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialSandBox);
			objSandBoxArena.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			objSandBoxArena.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
			objSandBoxArena.get<Transform>().SetLocalScale(0.25f, 0.25f, 0.25f);
		}
		
		GameObject objraArena = Arena1->CreateEntity("roundabout");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Arena1/RoundAbout.obj");
			objraArena.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialRA);
			objraArena.get<Transform>().SetLocalPosition(2.0f, 3.0f, 2.0f);
			objraArena.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
			objraArena.get<Transform>().SetLocalScale(0.25f, 0.25f, 0.25f);
		}

		GameObject objpinwheelArena = Arena1->CreateEntity("pinwheel");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Arena1/PinWheel.obj");
			objpinwheelArena.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialPinwheel);
			objpinwheelArena.get<Transform>().SetLocalPosition(3.0f, 0.0f, 2.0f);
			objpinwheelArena.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
			objpinwheelArena.get<Transform>().SetLocalScale(0.25f, 0.25f, 0.25f);
		}

		GameObject objTables = Arena1->CreateEntity("table");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Arena1/Table.obj");
			objTables.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialTable);
			objTables.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			objTables.get<Transform>().SetLocalRotation(90.0f, 0.0f, 270.0f);
			objTables.get<Transform>().SetLocalScale(0.25f, 0.25f, 0.25f);
		}
		
		/*GameObject objBenches = Arena1->CreateEntity("Benches");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Arena1/Table.obj");
			objBenches.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialTable);
			objBenches.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			objBenches.get<Transform>().SetLocalRotation(90.0f, 0.0f, 270.0f);
			objBenches.get<Transform>().SetLocalScale(0.25f, 0.25f, 0.25f);
		}*/
		
		GameObject objBalloons = Arena1->CreateEntity("Balloons");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Arena1/Balloons.obj");
			objBalloons.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialBalloons);
			objBalloons.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			objBalloons.get<Transform>().SetLocalRotation(90.0f, 0.0f, 270.0f);
			objBalloons.get<Transform>().SetLocalScale(0.23f, 0.25f, 0.25f);
		}
		
		GameObject objTrees = Arena1->CreateEntity("trees");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Arena1/Trees.obj");
			objTrees.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialtrees);
			objTrees.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			objTrees.get<Transform>().SetLocalRotation(90.0f, 0.0f, 270.0f);
			objTrees.get<Transform>().SetLocalScale(0.27f, 0.27f, 0.27f);
		}
		
		GameObject objFlowers = Arena1->CreateEntity("flowers");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Arena1/Flower.obj");
			objFlowers.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialflowers);
			objFlowers.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			objFlowers.get<Transform>().SetLocalRotation(90.0f, 0.0f, 90.0f);
			objFlowers.get<Transform>().SetLocalScale(0.23f, 0.23f, 0.23f);
		}
		
		GameObject objHedge = Arena1->CreateEntity("Hedge");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Arena1/Hedge.obj");
			objHedge.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialHedge);
			objHedge.get<Transform>().SetLocalPosition(0.0f, 0.0f, 3.0f);
			objHedge.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			objHedge.get<Transform>().SetLocalScale(0.25f, 0.25f, 0.25f);
		}
		
		GameObject objGroundArena = Arena1->CreateEntity("Ground");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Arena1/Ground.obj");
			objGroundArena.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialGroundArena);
			objGroundArena.get<Transform>().SetLocalPosition(0.0f, 0.0f, -4.0f);
			objGroundArena.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			objGroundArena.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
		}	
		
		GameObject objBottleText1 = Arena1->CreateEntity("BottleUItext");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Arena1/BottleText.obj");
			objBottleText1.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialBottleyellow);
			objBottleText1.get<Transform>().SetLocalPosition(12.0f, 14.0f, 2.0f);
			objBottleText1.get<Transform>().SetLocalRotation(0.0f, 180.0f, 180.0f);
			objBottleText1.get<Transform>().SetLocalScale(3.0f, 3.0f, 3.0f);
		}

		GameObject objBottleText2 = Arena1->CreateEntity("BottleUItext");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Arena1/BottleText.obj");
			objBottleText2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(materialBottlepink);
			objBottleText2.get<Transform>().SetLocalPosition(-4.0f, 14.0f, 2.0f);
			objBottleText2.get<Transform>().SetLocalRotation(0.0f, 180.0f, 180.0f);
			objBottleText2.get<Transform>().SetLocalScale(3.0f, 3.0f, 3.0f);
		}
		#pragma endregion Arena1 Objects

		#pragma region Skybox
		/////////////////////////////////// SKYBOX ///////////////////////////////////////////////
		{
			// Load our shaders
			Shader::sptr skybox = std::make_shared<Shader>();
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.vert.glsl", GL_VERTEX_SHADER);
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.frag.glsl", GL_FRAGMENT_SHADER);
			skybox->Link();

			ShaderMaterial::sptr skyboxMat = ShaderMaterial::Create();
			skyboxMat->Shader = skybox;  
			skyboxMat->Set("s_Environment", environmentMap);
			skyboxMat->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));
			skyboxMat->RenderLayer = 100;

			MeshBuilder<VertexPosNormTexCol> mesh;
			MeshFactory::AddIcoSphere(mesh, glm::vec3(0.0f), 1.0f);
			MeshFactory::InvertFaces(mesh);
			VertexArrayObject::sptr meshVao = mesh.Bake();
			
			GameObject skyboxObj = scene->CreateEntity("skybox");  
			skyboxObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			skyboxObj.get_or_emplace<RendererComponent>().SetMesh(meshVao).SetMaterial(skyboxMat);
		}
		////////////////////////////////////////////////////////////////////////////////////////
		#pragma endregion Skybox

		// We'll use a vector to store all our key press events for now (this should probably be a behaviour eventually)
		std::vector<KeyPressWatcher> keyToggles;
		{
			// This is an example of a key press handling helper. Look at InputHelpers.h an .cpp to see
			// how this is implemented. Note that the ampersand here is capturing the variables within
			// the scope. If you wanted to do some method on the class, your best bet would be to give it a method and
			// use std::bind
			keyToggles.emplace_back(GLFW_KEY_T, [&]() { cameraObject.get<Camera>().ToggleOrtho(); });

			controllables.push_back(objDunce);
			controllables.push_back(objDuncet);

			keyToggles.emplace_back(GLFW_KEY_KP_ADD, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao++;
				if (selectedVao >= controllables.size())
					selectedVao = 0;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});
			keyToggles.emplace_back(GLFW_KEY_KP_SUBTRACT, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao--;
				if (selectedVao < 0)
					selectedVao = controllables.size() - 1;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});

			keyToggles.emplace_back(GLFW_KEY_Y, [&]() {
				auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
				behaviour->Relative = !behaviour->Relative;
				});
		}
		
		InitImGui();

		// Initialize our timing instance and grab a reference for our use
		Timing& time = Timing::Instance();
		time.LastFrame = glfwGetTime();

		///// Game loop /////
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();

			// Update the timing
			time.CurrentFrame = glfwGetTime();
			time.DeltaTime = static_cast<float>(time.CurrentFrame - time.LastFrame);

			time.DeltaTime = time.DeltaTime > 1.0f ? 1.0f : time.DeltaTime;

			// Update our FPS tracker data
			fpsBuffer[frameIx] = 1.0f / time.DeltaTime;
			frameIx++;
			if (frameIx >= 128)
				frameIx = 0;

			// We'll make sure our UI isn't focused before we start handling input for our game
			if (!ImGui::IsAnyWindowFocused()) {
				// We need to poll our key watchers so they can do their logic with the GLFW state
				// Note that since we want to make sure we don't copy our key handlers, we need a const
				// reference!
				for (const KeyPressWatcher& watcher : keyToggles) {
					watcher.Poll(window);
				}
			}

			// Clear the screen
			glClearColor(0.08f, 0.17f, 0.31f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			#pragma region Rendering seperate scenes

			// Start by assuming no shader or material is applied
			Shader::sptr current = nullptr;
			ShaderMaterial::sptr currentMat = nullptr;

			// Grab out camera info from the camera object
			Transform& camTransform = cameraObject.get<Transform>();
			glm::mat4 view = glm::inverse(camTransform.LocalTransform());
			glm::mat4 projection = cameraObject.get<Camera>().GetProjection();
			glm::mat4 viewProjection = projection * view;

			#pragma region Menu
			if (Application::Instance().ActiveScene == Menu) {

				if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
				{
					Application::Instance().ActiveScene = Arena1;//just to test change to arena1 later
				}
				
				if (glfwGetKey(window,GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS)
				{
					Application::Instance().ActiveScene = scene;//just to test change to arena1 later
				}

				// Iterate over all the behaviour binding components
				Menu->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
					// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
					for (const auto& behaviour : binding.Behaviours) {
						if (behaviour->Enabled) {
							behaviour->Update(entt::handle(Menu->Registry(), entity));
						}
					}
					});

				// Update all world matrices for this frame
				Menu->Registry().view<Transform>().each([](entt::entity entity, Transform& t) {
					t.UpdateWorldMatrix();
					});

				// Sort the renderers by shader and material, we will go for a minimizing context switches approach here,
			// but you could for instance sort front to back to optimize for fill rate if you have intensive fragment shaders
				renderGroupMenu.sort<RendererComponent>([](const RendererComponent& l, const RendererComponent& r) {
					// Sort by render layer first, higher numbers get drawn last
					if (l.Material->RenderLayer < r.Material->RenderLayer) return true;
					if (l.Material->RenderLayer > r.Material->RenderLayer) return false;

					// Sort by shader pointer next (so materials using the same shader run sequentially where possible)
					if (l.Material->Shader < r.Material->Shader) return true;
					if (l.Material->Shader > r.Material->Shader) return false;

					// Sort by material pointer last (so we can minimize switching between materials)
					if (l.Material < r.Material) return true;
					if (l.Material > r.Material) return false;

					return false;
					});

				// Iterate over the render group components and draw them
				renderGroupMenu.each([&](entt::entity e, RendererComponent& renderer, Transform& transform) {
					// If the shader has changed, set up it's uniforms
					if (current != renderer.Material->Shader) {
						current = renderer.Material->Shader;
						current->Bind();
						SetupShaderForFrame(current, view, projection);
					}
					// If the material has changed, apply it
					if (currentMat != renderer.Material) {
						currentMat = renderer.Material;
						currentMat->Apply();
					}
					// Render the mesh
					RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform);
					});
			}
			#pragma endregion Menu

			#pragma region scene(testing)
			if (Application::Instance().ActiveScene == scene) {

				if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
				{
					Application::Instance().ActiveScene = Pause;
				}

				// Draw our ImGui content
				RenderImGui();

				//Hit detection test
				if (Collision(objDuncet.get<Transform>(), Hitboxes[0].get<Transform>())) {
					PlayerMovement::Player2vswall(objDuncet.get<Transform>(), time.DeltaTime);
				}

				//Player Movemenet(seperate from camera controls)
				PlayerMovement::player1and2move(objDunce.get<Transform>(), objDuncet.get<Transform>(), time.DeltaTime);

				// Iterate over all the behaviour binding components
				scene->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
					// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
					for (const auto& behaviour : binding.Behaviours) {
						if (behaviour->Enabled) {
							behaviour->Update(entt::handle(scene->Registry(), entity));
						}
					}
				});

				// Update all world matrices for this frame
				scene->Registry().view<Transform>().each([](entt::entity entity, Transform& t) {
					t.UpdateWorldMatrix();
				});

				// Sort the renderers by shader and material, we will go for a minimizing context switches approach here,
			// but you could for instance sort front to back to optimize for fill rate if you have intensive fragment shaders
				renderGroup.sort<RendererComponent>([](const RendererComponent& l, const RendererComponent& r) {
					// Sort by render layer first, higher numbers get drawn last
					if (l.Material->RenderLayer < r.Material->RenderLayer) return true;
					if (l.Material->RenderLayer > r.Material->RenderLayer) return false;

					// Sort by shader pointer next (so materials using the same shader run sequentially where possible)
					if (l.Material->Shader < r.Material->Shader) return true;
					if (l.Material->Shader > r.Material->Shader) return false;

					// Sort by material pointer last (so we can minimize switching between materials)
					if (l.Material < r.Material) return true;
					if (l.Material > r.Material) return false;

					return false;
					});

				// Iterate over the render group components and draw them
				renderGroup.each([&](entt::entity e, RendererComponent& renderer, Transform& transform) {
					// If the shader has changed, set up it's uniforms
					if (current != renderer.Material->Shader) {
						current = renderer.Material->Shader;
						current->Bind();
						SetupShaderForFrame(current, view, projection);
					}
					// If the material has changed, apply it
					if (currentMat != renderer.Material) {
						currentMat = renderer.Material;
						currentMat->Apply();
					}
					// Render the mesh
					RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform);
				});
			}
			#pragma endregion scene(testing)

			#pragma region Arena 1 scene stuff
			if (Application::Instance().ActiveScene == Arena1)
			{
				camTransform = cameraObject.get<Transform>().SetLocalPosition(0, 0, 17).SetLocalRotation(0, 0, 180);
				view = glm::inverse(camTransform.LocalTransform());
				projection = cameraObject.get<Camera>().GetProjection();
				viewProjection = projection * view;
				if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
				{
					Application::Instance().ActiveScene = Pause;
				}

				//Player Movemenet(seperate from camera controls)
				PlayerMovement::player1and2move(objDunceArena.get<Transform>(), objDuncetArena.get<Transform>(), time.DeltaTime);

				//Hit detection test
				/*if (Collision(objDuncet.get<Transform>(), objTables.get<Transform>())) {
					PlayerMovement::Player2vswall(objDuncetArena.get<Transform>(), time.DeltaTime);
				}*/

				// Iterate over all the behaviour binding components
				Arena1->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
					// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
					for (const auto& behaviour : binding.Behaviours) {
						if (behaviour->Enabled) {
							behaviour->Update(entt::handle(Arena1->Registry(), entity));
						}
					}
				});

				Arena1->Registry().view<Transform>().each([](entt::entity entity, Transform& t) {
					t.UpdateWorldMatrix();
				});

				renderGroupArena.sort<RendererComponent>([](const RendererComponent& l, const RendererComponent& r) {
					// Sort by render layer first, higher numbers get drawn last
					if (l.Material->RenderLayer < r.Material->RenderLayer) return true;
					if (l.Material->RenderLayer > r.Material->RenderLayer) return false;

					// Sort by shader pointer next (so materials using the same shader run sequentially where possible)
					if (l.Material->Shader < r.Material->Shader) return true;
					if (l.Material->Shader > r.Material->Shader) return false;

					// Sort by material pointer last (so we can minimize switching between materials)
					if (l.Material < r.Material) return true;
					if (l.Material > r.Material) return false;

					return false;
				});
				renderGroupArena.each([&](entt::entity e, RendererComponent& renderer, Transform& transform) {
					// If the shader has changed, set up it's uniforms
					if (current != renderer.Material->Shader) {
						current = renderer.Material->Shader;
						current->Bind();
						SetupShaderForFrame(current, view, projection);
					}
					// If the material has changed, apply it
					if (currentMat != renderer.Material) {
						currentMat = renderer.Material;
						currentMat->Apply();
					}
					// Render the mesh
					RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform);
				});
			}
			#pragma endregion Arena 1 scene stuff

			#pragma region Pause
			if (Application::Instance().ActiveScene == Pause) {

				if (glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS)
				{
					Application::Instance().ActiveScene = scene;//just to test change to arena1 later
				}

				if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
				{
					Application::Instance().ActiveScene = Arena1;//just to test change to arena1 later
				}

				// Iterate over all the behaviour binding components
				Pause->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
					// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
					for (const auto& behaviour : binding.Behaviours) {
						if (behaviour->Enabled) {
							behaviour->Update(entt::handle(Pause->Registry(), entity));
						}
					}
					});

				// Update all world matrices for this frame
				Pause->Registry().view<Transform>().each([](entt::entity entity, Transform& t) {
					t.UpdateWorldMatrix();
					});

				// Sort the renderers by shader and material, we will go for a minimizing context switches approach here,
			// but you could for instance sort front to back to optimize for fill rate if you have intensive fragment shaders
				renderGroupPause.sort<RendererComponent>([](const RendererComponent& l, const RendererComponent& r) {
					// Sort by render layer first, higher numbers get drawn last
					if (l.Material->RenderLayer < r.Material->RenderLayer) return true;
					if (l.Material->RenderLayer > r.Material->RenderLayer) return false;

					// Sort by shader pointer next (so materials using the same shader run sequentially where possible)
					if (l.Material->Shader < r.Material->Shader) return true;
					if (l.Material->Shader > r.Material->Shader) return false;

					// Sort by material pointer last (so we can minimize switching between materials)
					if (l.Material < r.Material) return true;
					if (l.Material > r.Material) return false;

					return false;
					});

				// Iterate over the render group components and draw them
				renderGroupPause.each([&](entt::entity e, RendererComponent& renderer, Transform& transform) {
					// If the shader has changed, set up it's uniforms
					if (current != renderer.Material->Shader) {
						current = renderer.Material->Shader;
						current->Bind();
						SetupShaderForFrame(current, view, projection);
					}
					// If the material has changed, apply it
					if (currentMat != renderer.Material) {
						currentMat = renderer.Material;
						currentMat->Apply();
					}
					// Render the mesh
					RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform);
					});
			}
			#pragma endregion Pause

			#pragma endregion Rendering seperate scenes
			
			scene->Poll();
			glfwSwapBuffers(window);
			time.LastFrame = time.CurrentFrame;
		}

		// Nullify scene so that we can release references
		Application::Instance().ActiveScene = nullptr;
		ShutdownImGui();
	}	

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}