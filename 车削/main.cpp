#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.h"
#include "camera.h"
#include "model.h"
#include <iostream>
#include <vector>
#include <string>
#include <ctime>


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void loadPBRtextures();
unsigned int loadTexture(const char *path);
void initCylinder();  //����Բ������Ϣ��ʼ��Բ����,����VAO,VBO�Ͷ�������
int FirstUnusedParticle();  //�ҵ�particles�����е�һ�������������±�
void initParticle(int index); //���������±��ʼ������


// ��Ļ���
unsigned int WIN_WIDTH = 800;
unsigned int WIN_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 1.0f));

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// pbr����
const unsigned int PBR_TYPES = 2;
unsigned int PBR_type = 0;
string PBRtypes[PBR_TYPES] = { "rusted_iron","wood" };
unsigned int albedo[PBR_TYPES];
unsigned int normal[PBR_TYPES];
unsigned int metallic[PBR_TYPES];
unsigned int roughness[PBR_TYPES];
unsigned int ao[PBR_TYPES];


//Բ������Ϣ,ע��Ӧ����double������float,���⾫�Ȳ������³�������
const double cylinderRadius = 0.4;
const double cylinderLength = 1.6;
const int angleStep = 2;			//�зֽǶ�����
const double lengthStep = 0.001;	//�зֳ�������
const double radiusStep = 0.001;	//�뾶����
vector<int> radiusArray;			//�뾶����,��ΪradiusStep��������
vector<int> radiusMinArray;			//�뾶��Сֵ����,�涨�뾶����Сֵ,���ڱ��������ߵ�����
vector<glm::vec3> allPoints;		//���е�����,�������㡢����������������
vector<unsigned int> indices;		//��������

unsigned int cylinderVAO;
unsigned int cylinderVBO;
unsigned int vertexNum; //��������


//����Ӧ�Ĳü�����(��׼���豸����,��ΧΪ-1~1),��ʼʱ������Բ�Ĵ�
const double clipX0 = 0.62, clipY0 = -0.2;  //�����ʼλ��
double clipX = clipX0, clipY = clipY0;  //������λ��
bool isCut = false;  //�Ƿ�����
int mode = 0;  //ģʽ,0��ʾ������ģʽ,����ָ������������,1��ʾ����ģʽ


//����ϵͳ
struct Particle {
	glm::vec3 position, velocity;
	float life;

	Particle() :position(0.0f), velocity(0.0f), life(1.0f) {}
};
vector<Particle> particles;			//��������
vector<glm::mat4> modelMatrices;	//ģ�;�������
const int PARTICLE_NUM = 2000;		//��������
const int NEW_PARTICLE_NUM = 8;		//�²�����������
const double GRAVITY = 9.8 / 2.0;	//����
const double xzVelorityMax = 0.6;	//x,z�����ϵ��ٶ����ֵ
const double fadeMax = 0.01;		//��������ÿ֡���������ֵ
bool isLeft = false;				//����x������ٶ��Ƿ�����


//Bezier����
vector<glm::vec2> BezierPoints;     //Լ����
vector<glm::vec2> BezierCurvePoints;//���ߵ�,�̶�Ϊ1001��

unsigned int bezierVAO, bezierVBO, bezierCurveVAO, bezierCurveVBO;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// ����openGLȫ������
	// -----------------------------
	srand(glfwGetTime());
	glEnable(GL_DEPTH_TEST);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glad_glPointSize(4);
	glad_glLineWidth(2);


	// ����ϵͳ��ʼ��
	// ----------------
	for (int i = 0; i < PARTICLE_NUM; ++i) {
		particles.push_back(Particle());
		initParticle(i);
	}


	// ������������,VAO,VBO
	// ----------------------------

	initCylinder();
	cout << "points:" << vertexNum << endl;

	float backgroundVertex[] = {
		-1.0f,-1.0f,  0.0f,0.0f,
		 1.0f,-1.0f,  1.0f,0.0f,
		 1.0f, 1.0f,  1.0f,1.0f,

		-1.0f,-1.0f,  0.0f,0.0f,
		 1.0f, 1.0f,  1.0f,1.0f,
		-1.0f, 1.0f,  0.0f,1.0f,
	};

	float particleVertex[] = {
		0.0f,0.0f,0.0f,  0.0f,0.0f,
		0.5f,0.0f,0.0f,  1.0f,0.0f,
		0.5f,0.5f,0.0f,  1.0f,1.0f,

		0.0f,0.0f,0.0f,  0.0f,0.0f,
		0.5f,0.5f,0.0f,  1.0f,1.0f,
		0.0f,0.5f,0.0f,  1.0f,0.0f,
	};

	for (int i = 0; i < PARTICLE_NUM; ++i) {
		modelMatrices.push_back(glm::mat4(1.0f));
	}

	vector<float> particleLifes;
	for (int i = 0; i < PARTICLE_NUM; ++i) {
		particleLifes.push_back(particles[i].life);
	}

	//����ϵͳ
	unsigned int particleVAO, particleVBO, modelMatrixVBO;
	glGenVertexArrays(1, &particleVAO);
	glGenBuffers(1, &particleVBO);
	glGenBuffers(1, &modelMatrixVBO);
	glBindVertexArray(particleVAO);
	glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(particleVertex), particleVertex, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, modelMatrixVBO);
	glBufferData(GL_ARRAY_BUFFER, PARTICLE_NUM * sizeof(glm::mat4), &modelMatrices[0], GL_STREAM_DRAW);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(0));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));
	glEnableVertexAttribArray(5);
	//��ÿ��ʵ��ʹ��ģ�;���,�����Ƕ�ÿ������
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);
	glBindVertexArray(0);

	//����ͼƬ
	unsigned int bgVAO, bgVBO;
	glGenVertexArrays(1, &bgVAO);
	glGenBuffers(1, &bgVBO);
	glBindVertexArray(bgVAO);
	glBindBuffer(GL_ARRAY_BUFFER, bgVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(backgroundVertex), backgroundVertex, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	//Bezier����
	glGenVertexArrays(1, &bezierVAO);
	glGenVertexArrays(1, &bezierCurveVAO);
	glGenBuffers(1, &bezierVBO);
	glGenBuffers(1, &bezierCurveVBO);

	//Լ����
	glBindVertexArray(bezierVAO);
	glBindBuffer(GL_ARRAY_BUFFER, bezierVBO);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	//Bezier���ߵ�
	glBindVertexArray(bezierCurveVAO);
	glBindBuffer(GL_ARRAY_BUFFER, bezierCurveVBO);
	glBufferData(GL_ARRAY_BUFFER, 1001 * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);   //bezier���ߵ�̶�Ϊ1001��
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);



	// ��������
	// -------------
	unsigned int bgTexture = loadTexture("resources/textures/background.bmp");



	//����pbr��������ʱ��
	clock_t startTime, endTime;
	startTime = clock();//��ʱ��ʼ

	loadPBRtextures();

	endTime = clock();//��ʱ����
	cout << "The run time is: " << (double)(endTime - startTime) / (time_t)1000 << "s" << endl;



	// ��ɫ������
	// --------------------
	Shader cylinderShader_pbr("cylinder.vs", "cylinder_pbr.fs");
	Shader modelShader("model.vs", "model.fs");
	Shader particleShader("particle.vs", "particle.fs");
	Shader bgShader("background.vs", "background.fs");
	Shader bezierShader("bezier.vs", "bezier.fs");

	//pbr����ģ��
	cylinderShader_pbr.use();
	cylinderShader_pbr.setInt("albedoMap", 0);
	cylinderShader_pbr.setInt("normalMap", 1);
	cylinderShader_pbr.setInt("metallicMap", 2);
	cylinderShader_pbr.setInt("roughnessMap", 3);
	cylinderShader_pbr.setInt("aoMap", 4);


	// pbr�ƹ�
	// ----------
	glm::vec3 lightPositions[] = {
		glm::vec3(0.0f, 0.0f, 4.0f)
	};
	glm::vec3 lightColors[] = {  //��Դ��ɫ:150.0f��ʾrgb�е�1.0f
		glm::vec3(150.0f, 150.0f, 150.0f),
	};


	// ����3dģ��:����
	// -----------
	Model myModel("resources/models/turningTool/turningTool.3ds");


	// ģ�͡���ͼ��ͶӰ����
	// ------------------------
	glm::mat4 model, view, projection;
	// ��ת�Ƕ�
	float angle = 0.0f;


	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// ����
		// -----
		processInput(window);

		// ��Ⱦ
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		// ���þ���
		// ---------
		model = glm::mat4(1.0f);
		view = camera.GetViewMatrix();
		float orthoSize = 1.0f;
		projection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, orthoSize, -0.0f);
		projection = glm::mat4(1.0f);  //ͶӰ��������Ϊ��λ����,��ΪĬ�ϵ�����ͶӰ

		// ��Բ���壺pbrģ��
		// ------------------------
		cylinderShader_pbr.use();
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.5f, 0.0f, 0.0f));
		//Բ������ת
		model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		cylinderShader_pbr.setMat4("model", model);
		cylinderShader_pbr.setMat4("view", view);
		cylinderShader_pbr.setMat4("projection", projection);
		cylinderShader_pbr.setVec3("viewPos", camera.Position);
		//����pbr��������
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, albedo[PBR_type]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, normal[PBR_type]);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, metallic[PBR_type]);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, roughness[PBR_type]);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, ao[PBR_type]);
		glBindVertexArray(cylinderVAO);
		glDrawElements(GL_TRIANGLES, vertexNum, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		//����pbr�ĵ��Դλ��(��Ϊ������Դ,���4��)
		for (unsigned int i = 0; i < sizeof(lightPositions) / sizeof(lightPositions[0]); ++i) {
			glm::vec3 newPos = lightPositions[i] + glm::vec3(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0);
			newPos = lightPositions[i];
			cylinderShader_pbr.setVec3("lightPositions[" + std::to_string(i) + "]", newPos);
			cylinderShader_pbr.setVec3("lightColors[" + std::to_string(i) + "]", lightColors[i]);
		}


		// ������ϵͳ
		// -------------
		if (isCut) {
			particleShader.use();
			particleShader.setMat4("view", view);
			particleShader.setMat4("projection", projection);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, albedo[PBR_type]);
			glBindVertexArray(particleVAO);
			glBindBuffer(GL_ARRAY_BUFFER, modelMatrixVBO);

			int UsedParticle = 0;
			modelMatrices.clear();
			for (int i = 0; i < PARTICLE_NUM; ++i) {
				Particle p = particles[i];
				if ((isLeft&&p.velocity.x > 0) || (!isLeft&&p.velocity.x < 0)) {
					p.velocity.x = -p.velocity.x;
					p.position.x = -p.position.x;
				}

				if (p.life > 0.0f) {
					model = glm::mat4(1.0f);
					model = glm::translate(model, glm::vec3(clipX - clipX0 + 0.5, clipY - clipY0, 0.0f)); //����ϵͳ������ƶ����ƶ�
					model = glm::translate(model, p.position);
					modelMatrices.push_back(model);
					UsedParticle++;
				}
			}
			glBufferSubData(GL_ARRAY_BUFFER, 0, UsedParticle * sizeof(glm::mat4), &modelMatrices[0]);
			//ֻ��life>0.0f������,��ʵ������������(��Ȼ��û����������,��ΪglBufferSubData�ֳ�Ϊ������ƿ��)
			glDrawArraysInstanced(GL_TRIANGLES, 0, 6, UsedParticle);
			glBindVertexArray(0);
		}


		// ��3dģ��
		// -------------
		modelShader.use();
		model = glm::mat4(1.0f);
		//ֱ��������������
		if (mode == 1) {
			model = glm::translate(model, glm::vec3(clipX, clipY, 0.0f));
		}
		else {
			model = glm::translate(model, glm::vec3(clipX0, clipY0, 0.0f));
		}
		//��ת����ȷ��λ
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.02f));
		modelShader.setMat4("model", model);
		myModel.Draw(modelShader);


		// ������ͼƬ
		// -------------
		glDepthFunc(GL_LEQUAL);   //���ñ�������������������(������)����
		bgShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, bgTexture);
		glBindVertexArray(bgVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS);


		// ��bezier����
		// -----------------
		bezierShader.use();

		//��bezier���ߵ�Լ����
		glBindVertexArray(bezierVAO);
		glDrawArrays(GL_POINTS, 0, BezierPoints.size());
		glBindVertexArray(0);

		//��bezier����
		if (BezierPoints.size() == 4) {
			glBindVertexArray(bezierCurveVAO);
			glDrawArrays(GL_LINE_STRIP, 0, BezierCurvePoints.size());
			glBindVertexArray(0);
		}


		// ���²���
		// --------------

		//������ת��
		angle += 0.8f;

		//��������ϵͳ

		//����������
		for (int i = 0; i < NEW_PARTICLE_NUM; ++i) {
			int unusedParticle = FirstUnusedParticle();
			initParticle(unusedParticle);
		}


		//������������
		for (int i = 0; i < PARTICLE_NUM; ++i) {
			Particle &p = particles[i];
			float dt = ((double)(rand() % 10) / 10.0)*fadeMax;
			p.life -= dt;
			if (p.life > 0.0f) {
				p.position += p.velocity * dt;
				p.velocity.y -= GRAVITY * dt;
			}
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &cylinderVAO);
	glDeleteVertexArrays(1, &particleVAO);
	glDeleteVertexArrays(1, &bgVAO);
	glDeleteVertexArrays(1, &bezierVAO);
	glDeleteVertexArrays(1, &bezierCurveVAO);
	glDeleteBuffers(1, &cylinderVBO);
	glDeleteBuffers(1, &particleVBO);
	glDeleteBuffers(1, &modelMatrixVBO);
	glDeleteBuffers(1, &bgVBO);
	glDeleteBuffers(1, &bezierVAO);
	glDeleteBuffers(1, &bezierCurveVAO);

	glfwTerminate();
	return 0;
}


void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);


	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
		PBR_type = 0;
	}
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
		PBR_type = 1;
	}

	if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
		mode = 1;  //ת��Ϊ����ģʽ
	}
	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
		mode = 0;  //ת��Ϊ������������ģʽ
	}
}


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (mode == 0) {
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			if (clipX <= clipX0) {
				int size = BezierPoints.size();
				if (size == 0 || (size < 4 && BezierPoints[size - 1].x != clipX)) {  //����4��Լ����
					BezierPoints.push_back(glm::vec2(clipX, clipY));
					glBindBuffer(GL_ARRAY_BUFFER, bezierVBO);
					glBufferSubData(GL_ARRAY_BUFFER, 0, BezierPoints.size() * sizeof(glm::vec2), &BezierPoints[0]);
					glBindBuffer(GL_ARRAY_BUFFER, 0);

					if (BezierPoints.size() == 4) {  //����4��Լ����,������ߵ�
						float curveX, curveY;
						glm::vec2 p0 = BezierPoints[0], p1 = BezierPoints[1], p2 = BezierPoints[2], p3 = BezierPoints[3];
						for (float t = 0.0f; t < 1.0f; t += 0.001f) {
							curveX = p0.x * glm::pow((1 - t), 3) + 3 * p1.x * t * glm::pow((1 - t), 2) + 3 * p2.x * t * t * (1 - t) + p3.x * pow(t, 3);
							curveY = p0.y * glm::pow((1 - t), 3) + 3 * p1.y * t * glm::pow((1 - t), 2) + 3 * p2.y * t * t * (1 - t) + p3.y * pow(t, 3);
							BezierCurvePoints.push_back(glm::vec2(curveX, curveY));
						}
						glBindBuffer(GL_ARRAY_BUFFER, bezierCurveVBO);
						glBufferSubData(GL_ARRAY_BUFFER, 0, BezierCurvePoints.size() * sizeof(glm::vec2), &BezierCurvePoints[0]);
						glBindBuffer(GL_ARRAY_BUFFER, 0);


						//���°뾶��Сֵ����
						int curveSize = BezierCurvePoints.size();
						int zIndex;  //��Բ����z�᷽���ϰ뾶������±�
						int newR;

						for (int i = 0; i <= curveSize - 1; ++i) {  //�������ߵ�֮��ļ��С��radiusStep,��ֱ�Ӷ�ÿ�����ߵ���°뾶��Сֵ����
							zIndex = (0.5f - BezierCurvePoints[i].x) / lengthStep;
							newR = (-BezierCurvePoints[i].y) / radiusStep;
							if (zIndex >= 0 && newR >= 0) {
								//����radiusMinArray
								radiusMinArray[zIndex] = newR;
							}
						}
					}
				}
			}
		}

		if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {  //���bezierԼ��������ߵ�
			BezierPoints.clear();
			BezierCurvePoints.clear();
		}
	}
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	if (width == 0 || height == 0) {
		WIN_WIDTH = width;
		WIN_HEIGHT = height;
	}
	glViewport(0, 0, width, height);
}


void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	//����Ļ����ת��Ϊ�ü�����(-1~1��Χ)
	double newClipX = xpos * 2.0 / (double)(WIN_WIDTH - 1) - 1.0;
	double newClipY = -ypos * 2.0 / (double)(WIN_HEIGHT - 1) + 1.0;

	//ģ������
	if (mode == 1) {   //����ģʽ
		if (newClipY > clipY0) { //ģ�Ͳ��ø���Բ��λ��
			newClipY = clipY0;
		}

		if (newClipX < clipX0 || clipX < clipX0) {
			int zStart = (clipX0 - (clipX > newClipX ? clipX : newClipX)) / lengthStep; //Բ����z�᷽���±�
			int zEnd = (clipX0 - (clipX > newClipX ? newClipX : clipX)) / lengthStep;
			int zGap = zEnd - zStart;
			//��ʼ���ֶ�Ӧ�İ뾶��radiusStep��������
			int R_start = (clipY0 - (clipX > newClipX ? clipY : newClipY)) / radiusStep;
			int R_end = (clipY0 - (clipX > newClipX ? newClipY : clipY)) / radiusStep;
			int R_gap = R_end - R_start;

			int stacks = cylinderLength / lengthStep;
			if (zStart >= 0 && zStart <= stacks && zEnd >= 0 && zEnd <= stacks && zGap >= 0 && R_start >= 0) {
				isCut = false;
				for (int i = zStart; i <= zEnd; ++i) {
					int newRadius;
					if (zGap == 0)
						newRadius = R_start;
					else
						newRadius = R_start + R_gap * (float)(i - zStart) / (float)zGap;

					if (newRadius < 0) newRadius = 0;
					//���°뾶����
					if (radiusArray[i] > newRadius&&radiusArray[i] > radiusMinArray[i]) {  //��С�뾶���ܱ����а뾶С
						isCut = true;
						radiusArray[i] = newRadius < radiusMinArray[i] ? radiusMinArray[i] : newRadius; //�°뾶��������а뾶С,��>=�涨����С�뾶
					}
				}

				//����VBO������
				glBindBuffer(GL_ARRAY_BUFFER, cylinderVBO);
				if (isCut) {
					if (newClipX < clipX) { //��������,�����ٶȷ���Ӧ����
						isLeft = false;
					}
					else {
						isLeft = true;
					}

					int slices = 360 / angleStep;
					for (int i = 0; i <= slices; ++i) {
						for (int j = zStart; j <= zEnd; ++j) {
							int pointOffset = (i*(stacks + 1) + j) * 3;
							glm::vec3 newPoint = allPoints[pointOffset];
							float alpha = i * angleStep;
							float newR = radiusArray[j] * radiusStep;
							newPoint.x = newR * (float)glm::cos(glm::radians(alpha));
							newPoint.y = newR * (float)glm::sin(glm::radians(alpha));
							glBufferSubData(GL_ARRAY_BUFFER, 3 * sizeof(float) * pointOffset, 2 * sizeof(float), &newPoint);
							allPoints[pointOffset] = newPoint;
						}

					}
				}
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
		}
	}


	//������������ƶ�
	clipX = newClipX;
	clipY = newClipY;
}


void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

// �������ͼ���pbr����
// ---------------------------------------------------
void loadPBRtextures()
{
	for (int type = 0; type < 1; ++type) {
		string folder = PBRtypes[type];
		string filepath1 = "resources/textures/pbr/" + folder + "/albedo.png";
		string filepath2 = "resources/textures/pbr/" + folder + "/normal.png";
		string filepath3 = "resources/textures/pbr/" + folder + "/metallic.png";
		string filepath4 = "resources/textures/pbr/" + folder + "/roughness.png";
		string filepath5 = "resources/textures/pbr/" + folder + "/ao.png";

		albedo[type] = loadTexture(filepath1.c_str());
		normal[type] = loadTexture(filepath2.c_str());
		metallic[type] = loadTexture(filepath3.c_str());
		roughness[type] = loadTexture(filepath4.c_str());
		ao[type] = loadTexture(filepath5.c_str());
	}
}


unsigned int loadTexture(char const * path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}
	stbi_set_flip_vertically_on_load(false);
	return textureID;
}



void initCylinder() {
	int slices = 360 / angleStep;  //Χ��z���ϸ��
	int stacks = cylinderLength / lengthStep;  //��z���ϸ��

	//��ʼ���뾶����Ͱ뾶��������
	unsigned int initRadius = cylinderRadius / radiusStep;
	for (int i = 0; i <= stacks; ++i) {
		radiusArray.push_back(initRadius);
	}

	for (int i = 0; i <= stacks; ++i) {
		radiusMinArray.push_back(0);  //��ʼʱ,�뾶��С������ȡ0
	}

	float R, alpha, x, y, z, texX, texY;

	for (int i = 0; i <= slices; i++) {
		for (int j = 0; j <= stacks; j++) {
			R = radiusArray[j] * radiusStep;
			alpha = i * angleStep;
			x = R * (float)glm::cos(glm::radians(alpha));
			y = R * (float)glm::sin(glm::radians(alpha));
			z = lengthStep * j;
			texX = (float)i / (float)slices;  //��������
			texY = (float)j / (float)stacks;

			//����
			glm::vec3 V(x, y, z);

			//����
			allPoints.push_back(V);
			allPoints.push_back(glm::vec3(V.x, V.y, 0.0f)); //������
			allPoints.push_back(glm::vec3(texX, texY, 0.0f));//��������(��2D,������allPointsֻ�ܽ���3D,�ʶ��û�õ�һ��float)


			//�����������
			if (i < slices && j < stacks) {
				unsigned int leftDown, leftUp, rightDown, rightUp;
				leftDown = i * (stacks + 1) + j;
				leftUp = i * (stacks + 1) + (j + 1);
				rightDown = (i + 1) * (stacks + 1) + j;
				rightUp = (i + 1) * (stacks + 1) + (j + 1);

				indices.push_back(leftDown);
				indices.push_back(leftUp);
				indices.push_back(rightUp);
				indices.push_back(leftDown);
				indices.push_back(rightUp);
				indices.push_back(rightDown);
			}
		}
	}

	vertexNum = indices.size();

	unsigned int VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*allPoints.size() * 3, &allPoints[0], GL_STREAM_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*vertexNum, &indices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	cylinderVAO = VAO;
	cylinderVBO = VBO;
}


int FirstUnusedParticle() {
	static int lastUsedParticle = 0;
	for (int i = lastUsedParticle; i < PARTICLE_NUM; ++i) {
		if (particles[i].life < 0.0f) {
			lastUsedParticle = i;
			return i;
		}
	}
	for (int i = 0; i < lastUsedParticle; ++i) {
		if (particles[i].life < 0.0f) {
			lastUsedParticle = i;
			return i;
		}
	}
	lastUsedParticle = 0;
	return 0;
}


void initParticle(int index) {
	Particle &p = particles[index];
	p.life = 1.0f;
	p.position = glm::vec3(0.0f, 0.0f, 0.0f);
	p.velocity.x = ((double)(rand() % 200 - 100) / 100.0)*xzVelorityMax;
	p.velocity.y = 0.0f;
	p.velocity.z = ((double)(rand() % 200 - 100) / 100.0)*xzVelorityMax;
}

