//TODO: acpect ratio = width/hight ; cofanie poprawne ; brak mozliwsoci opuszczenia kamery pod teren
//TODO: poruszac wieza i lufa, system czastek, poprawne swiatlo

/*
Niniejszy program jest wolnym oprogramowaniem; możesz go
rozprowadzać dalej i / lub modyfikować na warunkach Powszechnej
Licencji Publicznej GNU, wydanej przez Fundację Wolnego
Oprogramowania - według wersji 2 tej Licencji lub(według twojego
wyboru) którejś z późniejszych wersji.

Niniejszy program rozpowszechniany jest z nadzieją, iż będzie on
użyteczny - jednak BEZ JAKIEJKOLWIEK GWARANCJI, nawet domyślnej
gwarancji PRZYDATNOŚCI HANDLOWEJ albo PRZYDATNOŚCI DO OKREŚLONYCH
ZASTOSOWAŃ.W celu uzyskania bliższych informacji sięgnij do
Powszechnej Licencji Publicznej GNU.

Z pewnością wraz z niniejszym programem otrzymałeś też egzemplarz
Powszechnej Licencji Publicznej GNU(GNU General Public License);
jeśli nie - napisz do Free Software Foundation, Inc., 59 Temple
Place, Fifth Floor, Boston, MA  02110 - 1301  USA
*/

#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE


#include <GL/glew.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include "constants.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include "myCube.h"

// Pomocnicze zmienne
bool velocity = true;
bool shootBullet = false;
float move = 0.0f;
float modelRotation = 0;
float cameraRotation = 0;
float turretRotation = 0;
float gunRotation = 0;
float width = 800;
float hight = 800;
float aspectRatio = width/hight;
glm::mat4 Mbullet = glm::mat4(1.0f);

// Odkomentuj, żeby rysować kostkę
float* vertices = myCubeVertices;
float* normals = myCubeNormals;
float* texCoords = myCubeTexCoords;
float* texCoordsTerrain = myCubeTexCoordsTerrain;
float* colors = myCubeColors;
int vertexCount = myCubeVertexCount;

// Zmienne modelu
glm::vec3 modelPosition = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 modelDirection = glm::vec3(0.0f, 0.0f, 1.0f);

// Zmienne kamery
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f); // Wektor wskazujący górę
glm::vec3 cameraOffset = glm::vec3(0.0f, 7.0f, -16.0f);

ShaderProgram* sp;
ShaderProgram* skyBox;

// Struktura przechowująca informacje o modelu
struct Model
{
	std::vector<glm::vec4> vertices;
	std::vector<glm::vec4> normals;
	std::vector<glm::vec2> texCoords;
	std::vector<unsigned int> indices;
	int elementNum;
};

std::vector<Model> models; // Przechowuje modele składające się z kilku siatek

// Zmienne przechowujące inforamcje o modelu - nabój
std::vector<glm::vec4> verts;
std::vector<glm::vec4> norms;
std::vector<glm::vec2> texCoordss;
std::vector<unsigned int> indicess;


//Uchwyt do tekstury
GLuint tex, texTop, texBottom, texFront, texBack, texLeft, texRight, texStones, texGrass, texBullet;


// Wczytanie tekstury
GLuint readTexture(const char* filename) { //Deklaracja globalna
	GLuint tex;
	glActiveTexture(GL_TEXTURE0);
	//Wczytanie do pamięci komputera
	std::vector<unsigned char> image; //Alokuj wektor do wczytania obrazka
	unsigned width, height; //Zmienne do których wczytamy wymiary obrazka
	//Wczytaj obrazek
	unsigned error = lodepng::decode(image, width, height, filename);
	//Import do pamięci karty graficznej
	glGenTextures(1, &tex); //Zainicjuj jeden uchwyt
	glBindTexture(GL_TEXTURE_2D, tex); //Uaktywnij uchwyt
	//Wczytaj obrazek do pamięci KG skojarzonej z uchwytem
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	return tex;
}


// Wczytanie tekstury terenu z Mimpmapingiem
GLuint readTextureTerrain(const char* filename) { //Deklaracja globalna
	GLuint tex;
	glActiveTexture(GL_TEXTURE0);
	//Wczytanie do pamięci komputera
	std::vector<unsigned char> image; //Alokuj wektor do wczytania obrazka
	unsigned width, height; //Zmienne do których wczytamy wymiary obrazka
	//Wczytaj obrazek
	unsigned error = lodepng::decode(image, width, height, filename);
	//Import do pamięci karty graficznej
	glGenTextures(1, &tex); //Zainicjuj jeden uchwyt
	glBindTexture(GL_TEXTURE_2D, tex); //Uaktywnij uchwyt
	//Wczytaj obrazek do pamięci KG skojarzonej z uchwytem
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		GL_LINEAR_MIPMAP_LINEAR);
	return tex;
}


//Procedura obsługi błędów
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}


void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	// Ruch modelem - do przodu
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		modelPosition -= modelDirection * 0.1f;
		velocity = true; // Zmienna wskazująca czy czołg jedzie do przodu, czy do tyłu
	}

	// Ruch modelem - do tyłu
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		modelPosition += modelDirection * 0.1f;
		velocity = false;
	}
	
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE) {
	velocity = true;
	}


	// Obrót w lewo
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		if (velocity) modelRotation += 0.3f;
		else modelRotation -= 0.3f;

	}
		
	// Obrót w prawo
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		if (velocity) modelRotation -= 0.3f;
		else modelRotation += 0.3f;
	}

	// Aktualizacja wektora kierunku czołgu na podstawie aktualnej rotacji - "modelRotation"
	modelDirection = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(modelRotation), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));


	// Ruch kamerą - w lewo
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		cameraRotation -= 0.35f;
	}

	// Ruch kamerą - w prawo
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		cameraRotation += 0.35f;
	}
		
	// Ruch kamerą - w górę
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		cameraOffset.y += 0.1f;

	// Ruch kamerą - w dół
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		if (cameraOffset.y > 0.2) cameraOffset.y -= 0.1f;

	// Przybliżenie
	if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
		cameraOffset.z += 0.1f;

	// Oddalenie
	if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
		cameraOffset.z -= 0.1f;


	//Ruch wieżą - w lewo
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		turretRotation += 0.1f;

	//Ruch wieżą - w prawo
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		turretRotation -= 0.1f;


	//Ruch lufa - w dół
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
		if (gunRotation < 1.0f) gunRotation += 0.1f;
	}
		
	//Ruch lufa - w górę
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS){
		if (gunRotation > -2.0f) gunRotation -= 0.1f; 
	}
	
	// Strzał
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		shootBullet = true;
	}
	
}



// Wczytuje jeden model - nabój
void loadOneElement(std::string plik) {
	using namespace std;

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(plik,
		aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);
	cout << importer.GetErrorString() << endl;
	
	const aiMesh* mesh = scene->mMeshes[0];

	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {

		// Wierzchołki
		aiVector3D vertex = mesh->mVertices[i]; 
		verts.push_back(glm::vec4(vertex.x, vertex.y, vertex.z, 1));

		// Wektory znormalizowane
		aiVector3D normal = mesh->mNormals[i]; 
		norms.push_back(glm::vec4(normal.x, normal.y, normal.z, 0));

		// Współrzędne teksturowania
		aiVector3D texCoord = mesh->mTextureCoords[0][i];
		texCoordss.push_back(glm::vec2(texCoord.x, texCoord.y));
	}


	for (unsigned int l = 0; l < mesh->mNumFaces; l++) {
		const aiFace& face = mesh->mFaces[l]; //face to jeden z wielokatéw siatki
		for (unsigned int k = 0; k < face.mNumIndices; k++) {
			indicess.push_back(face.mIndices[k]);
		}
	}
}

void loadModel(std::string plik) {
	using namespace std;

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(plik,
		aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);
	cout << importer.GetErrorString() << endl;
 	for (unsigned int j = 0; j < scene->mNumMeshes; j++) {

		const aiMesh* mesh = scene->mMeshes[j];
		Model model;

		// Indeksowanie elementów modelu
		model.elementNum = j;

		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			
			// Wierzchołki
			aiVector3D vertex = mesh->mVertices[i];
			model.vertices.push_back(glm::vec4(vertex.x, vertex.y, vertex.z, 1));

			// Wektory znormalizowane
			aiVector3D normal = mesh->mNormals[i]; 
			model.normals.push_back(glm::vec4(normal.x, normal.y, normal.z, 0));

			// Współrzędne teksturowania
			aiVector3D texCoord = mesh->mTextureCoords[0][i];
			model.texCoords.push_back(glm::vec2(texCoord.x, texCoord.y));
		}

		for (unsigned int l = 0; l < mesh->mNumFaces; l++) {
			const aiFace& face = mesh->mFaces[l]; //face to jeden z wielokatéw siatki
			for (unsigned int k = 0; k < face.mNumIndices; k++) {
				model.indices.push_back(face.mIndices[k]);
			}
		}

		models.push_back(model);

	}
}



void windowResizeCallback(GLFWwindow* window, int width, int height) {
	if (height == 0) return;
	aspectRatio = (float)width / (float)height;
	glViewport(0, 0, width, height);
}

//Procedura inicjująca
void initOpenGLProgram(GLFWwindow* window) {
	//************Tutaj umieszczaj kod, który należy wykonać raz, na początku programu************
	glClearColor(0, 0.15, 0.3, 1);
	glEnable(GL_DEPTH_TEST);
	glfwSetWindowSizeCallback(window, windowResizeCallback);
	//glfwSetKeyCallback(window,keyCallback);

	// Wczytanie tekstur
	tex = readTexture("tank/tekstury/PzVl_Tiger_I.png");
	texTop = readTexture("skybox/top.png");
	texBottom = readTexture("skybox/bottom.png");
	texFront = readTexture("skybox/front.png");
	texBack = readTexture("skybox/back.png");
	texLeft = readTexture("skybox/left.png");
	texRight = readTexture("skybox/right.png");
	texStones = readTextureTerrain("stones.png");
	texGrass = readTextureTerrain("grass.png");
	texBullet = readTexture("metal_scratches.png");

	// Załadowanie modeli z pliku
	loadModel(std::string("tank/Tiger_I.obj"));
	loadOneElement(std::string("45.obj"));

	// Włączenie shaderów
	sp = new ShaderProgram("v_simplest.glsl", NULL, "f_simplest.glsl");
	skyBox = new ShaderProgram("v_skybox.glsl", NULL, "f_skybox.glsl");
}


//Zwolnienie zasobów zajętych przez program
void freeOpenGLProgram(GLFWwindow* window) {
	//************Tutaj umieszczaj kod, który należy wykonać po zakończeniu pętli głównej************
	glDeleteTextures(1, &tex);
	glDeleteTextures(1, &texTop);
	glDeleteTextures(1, &texBottom);
	glDeleteTextures(1, &texFront);
	glDeleteTextures(1, &texBack);
	glDeleteTextures(1, &texLeft);
	glDeleteTextures(1, &texRight);
	glDeleteTextures(1, &texStones);
	glDeleteTextures(1, &texGrass);
	glDeleteTextures(1, &texBullet);
	
	delete sp;
	delete skyBox;
}


// Funckja strzału
void shoot(glm::mat4 Mtemp) {

	if (move > 300) {
		shootBullet = false;
		move = 0;
		return;
	}

	// Modyfikuj Mbullet tylko na początku przed "wystrzałem"
	if (move < 1.0f) {
		Mbullet = Mtemp;
		Mbullet = glm::translate(Mbullet, glm::vec3(0.0f, 2.2f, 0.0f));
		Mbullet = glm::rotate(Mbullet, glm::radians(turretRotation), glm::vec3(0.0f, 1.0f, 0.0f));
		Mbullet = glm::rotate(Mbullet, glm::radians(gunRotation), glm::vec3(1.0f, 0.0f, 0.0f));
		Mbullet = glm::translate(Mbullet, glm::vec3(0.1f, 0.0f, 6.2f));
		Mbullet = glm::rotate(Mbullet, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	}
	
	// Przemieszczanie pocisku
	Mbullet = glm::translate(Mbullet, glm::vec3(0.0f, move, 0.0f));
	move += 1.5f;

	glVertexAttribPointer(sp->a("vertex"), 4, GL_FLOAT, false, 0, verts.data()); //Wskaż tablicę z danymi dla atrybutu vertex
	glVertexAttribPointer(sp->a("normal"), 4, GL_FLOAT, false, 0, norms.data()); //Wskaż tablicę z danymi dla atrybutu normal
	glVertexAttribPointer(sp->a("texCoord0"), 2, GL_FLOAT, false, 0, texCoordss.data());

	glBindTexture(GL_TEXTURE_2D, texBullet);
	glUniform1i(sp->u("textureMap0"), 0);
	glActiveTexture(GL_TEXTURE0);

	glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(Mbullet));
	glDrawElements(GL_TRIANGLES, indicess.size(), GL_UNSIGNED_INT, indicess.data());

	glDisableVertexAttribArray(sp->a("vertex"));  //Wyłącz przesyłanie danych do atrybutu vertex
	glDisableVertexAttribArray(sp->a("normal"));  //Wyłącz przesyłanie danych do atrybutu normal
	glDisableVertexAttribArray(sp->a("texCoord0")); 


}


// Funckja rysująca lufę i umorzyliwająca proszuanie lufą w górę i w dół
void drawGun(glm::mat4 Mtemp, const Model& model) {

	glm::mat4 Mgun = Mtemp;
	Mgun = glm::rotate(Mgun, glm::radians(turretRotation), glm::vec3(0.0f, 1.0f, 0.0f));
	Mgun = glm::rotate(Mgun, glm::radians(gunRotation), glm::vec3(1.0f, 0.0f, 0.0f));

	glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(Mgun));

	glEnableVertexAttribArray(sp->a("vertex"));  //Włącz przesyłanie danych do atrybutu vertex
	glVertexAttribPointer(sp->a("vertex"), 4, GL_FLOAT, false, 0, model.vertices.data()); //Wskaż tablicę z danymi dla atrybutu vertex

	glEnableVertexAttribArray(sp->a("texCoord0"));
	glVertexAttribPointer(sp->a("texCoord0"), 2, GL_FLOAT, false, 0, model.texCoords.data());

	glEnableVertexAttribArray(sp->a("normal"));  //Włącz przesyłanie danych do atrybutu normal
	glVertexAttribPointer(sp->a("normal"), 4, GL_FLOAT, false, 0, model.normals.data()); //Wskaż tablicę z danymi dla atrybutu normal

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1i(sp->u("textureMap0"), 0);

	glDrawElements(GL_TRIANGLES, model.indices.size(), GL_UNSIGNED_INT, model.indices.data());

	glDisableVertexAttribArray(sp->a("vertex"));  //Wyłącz przesyłanie danych do atrybutu vertex
	glDisableVertexAttribArray(sp->a("normal"));  //Wyłącz przesyłanie danych do atrybutu normal
	glDisableVertexAttribArray(sp->a("texCoord0"));
}


// Funckja rysująca wieżę i umorzyliwająca proszuanie nią na boki względem osi y
void drawTurret(glm::mat4 Mtemp, const Model &model) {

	glm::mat4 Mturret = Mtemp;
	Mturret = glm::rotate(Mturret, glm::radians(turretRotation), glm::vec3(0.0f, 1.0f, 0.0f));
	
	glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(Mturret));

	glEnableVertexAttribArray(sp->a("vertex"));  //Włącz przesyłanie danych do atrybutu vertex
	glVertexAttribPointer(sp->a("vertex"), 4, GL_FLOAT, false, 0, model.vertices.data()); //Wskaż tablicę z danymi dla atrybutu vertex

	glEnableVertexAttribArray(sp->a("texCoord0"));
	glVertexAttribPointer(sp->a("texCoord0"), 2, GL_FLOAT, false, 0, model.texCoords.data());

	glEnableVertexAttribArray(sp->a("normal"));  //Włącz przesyłanie danych do atrybutu normal
	glVertexAttribPointer(sp->a("normal"), 4, GL_FLOAT, false, 0, model.normals.data()); //Wskaż tablicę z danymi dla atrybutu normal

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1i(sp->u("textureMap0"), 0);

	glDrawElements(GL_TRIANGLES, model.indices.size(), GL_UNSIGNED_INT, model.indices.data());

	glDisableVertexAttribArray(sp->a("vertex"));  //Wyłącz przesyłanie danych do atrybutu vertex
	glDisableVertexAttribArray(sp->a("normal"));  //Wyłącz przesyłanie danych do atrybutu normal
	glDisableVertexAttribArray(sp->a("texCoord0"));

}


// Funckja rysująca niebo
void drawSky(glm::vec3 tankPosition) {

	glm::mat4 Msky = glm::mat4(1.f);
	Msky = glm::translate(Msky, tankPosition);
	Msky = glm::scale(Msky, glm::vec3(256.0f, 128.f, 256.f));
	Msky = glm::translate(Msky, glm::vec3(0.0f, -0.15f, 0.0f));

	glUniformMatrix4fv(skyBox->u("M"), 1, false, glm::value_ptr(Msky));

	glVertexAttribPointer(skyBox->a("vertex"), 4, GL_FLOAT, false, 0, myCubeVertices); //Wskaż tablicę z danymi dla atrybutu vertex
	glVertexAttribPointer(skyBox->a("texCoord0"), 2, GL_FLOAT, false, 0, myCubeTexCoords);

	glUniform1i(skyBox->u("textureMap0"), 0);
	glActiveTexture(GL_TEXTURE0);
	
	glBindTexture(GL_TEXTURE_2D, texFront);
	glDrawArrays(GL_TRIANGLES, 0, 6 ); //Narysuj obiekt

	glBindTexture(GL_TEXTURE_2D, texBack);
	glDrawArrays(GL_TRIANGLES, 6, 12); //Narysuj obiekt

	glBindTexture(GL_TEXTURE_2D, texBottom);
	glDrawArrays(GL_TRIANGLES, 12, 18); //Narysuj obiekt

	glBindTexture(GL_TEXTURE_2D, texTop);
	glDrawArrays(GL_TRIANGLES, 18, 24); //Narysuj obiekt

	glBindTexture(GL_TEXTURE_2D, texLeft);
	glDrawArrays(GL_TRIANGLES, 24, 30); //Narysuj obiekt

	glBindTexture(GL_TEXTURE_2D, texRight);
	glDrawArrays(GL_TRIANGLES, 30, 36); //Narysuj obiekt	
}


// Funkcja rysująca teren
void drawTerrain(glm::vec3 tankPosition) {
	
	glVertexAttribPointer(skyBox->a("vertex"), 4, GL_FLOAT, false, 0, myCubeVertices); //Wskaż tablicę z danymi dla atrybutu vertex
	glVertexAttribPointer(skyBox->a("texCoord0"), 2, GL_FLOAT, false, 0, myCubeTexCoordsTerrain);

	glBindTexture(GL_TEXTURE_2D, texStones);
	glUniform1i(skyBox->u("textureMap0"), 0);
	glActiveTexture(GL_TEXTURE0);

	glm::mat4 Mterr = glm::mat4(1.0f);
	Mterr = glm::scale(Mterr, glm::vec3(512.0f, 0.0f, 512.0f));

	glUniformMatrix4fv(skyBox->u("M"), 1, false, glm::value_ptr(Mterr));
	glDrawArrays(GL_TRIANGLES, 0, vertexCount); //Narysuj obiekt

}


//Procedura rysująca zawartość sceny
void drawScene(GLFWwindow* window) {
	//************Tutaj umieszczaj kod rysujący obraz******************l
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Obliczenie macierzy P
	glm::mat4 P = glm::perspective(60.0f * PI / 180.0f, aspectRatio, 0.01f, 1024.0f); //Wylicz macierz rzutowania

	// Obliczenie nowej pozycji kamery
	glm::vec4 cameraOffset4(cameraOffset, 1.0f); // Konwertowanie wektora cameraOffset do postaci 4D
	glm::vec4 cameraOffset4_v2 = glm::rotate(glm::mat4(1.0f), glm::radians(cameraRotation), glm::vec3(0.0f, 1.0f, 0.0f)) * cameraOffset4;
	glm::vec4 rotatedOffset = glm::rotate(glm::mat4(1.0f), glm::radians(modelRotation), glm::vec3(0.0f, 1.0f, 0.0f)) * cameraOffset4_v2;
	glm::vec3 newCameraPosition = modelPosition + glm::vec3(rotatedOffset);

	// Obliczenie macierzy widoku
	glm::mat4 V = glm::lookAt(newCameraPosition, modelPosition, cameraUp);


	skyBox->use();

	glEnableVertexAttribArray(skyBox->a("vertex"));   //Włącz przesyłanie danych do atrybutu vertex
	glEnableVertexAttribArray(skyBox->a("texCoord0")); //Włącz przesyłanie danych do atrybutu texCoord0

	//Przeslij parametry do karty graficznej
	glUniformMatrix4fv(skyBox->u("P"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(skyBox->u("V"), 1, false, glm::value_ptr(V));


	glDepthMask(GL_FALSE);
	drawSky(modelPosition);
	glDepthMask(GL_TRUE);

	glDepthMask(GL_FALSE);
	drawTerrain(modelPosition);
	glDepthMask(GL_TRUE);

	glm::mat4 M = glm::mat4(1.0f);
	M = glm::translate(M, modelPosition);
	M = glm::rotate(M, glm::radians(modelRotation), glm::vec3(0.0f, 1.0f, 0.0f));

	sp->use();

	//Przeslij parametry do karty graficznej
	glUniformMatrix4fv(sp->u("P"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(sp->u("V"), 1, false, glm::value_ptr(V));

	// Wykonaj strzał jeśli wciśnięto "T"
	if (shootBullet) {
		shoot(M);
	}

	// Iteracja po elementach modelu
	for (const auto& model : models) {

		// Rysowanie lufy
		if (model.elementNum == 0) {
			drawGun(M, model);
			continue;
		}
		
		// Rysowanie wieży
		if (model.elementNum == 1) {
			drawTurret(M, model);
			continue;
		}


		glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(M));

		glEnableVertexAttribArray(sp->a("vertex"));  //Włącz przesyłanie danych do atrybutu vertex
		glVertexAttribPointer(sp->a("vertex"), 4, GL_FLOAT, false, 0, model.vertices.data()); //Wskaż tablicę z danymi dla atrybutu vertex

		glEnableVertexAttribArray(sp->a("texCoord0"));
		glVertexAttribPointer(sp->a("texCoord0"), 2, GL_FLOAT, false, 0, model.texCoords.data());

		glEnableVertexAttribArray(sp->a("normal"));  //Włącz przesyłanie danych do atrybutu normal
		glVertexAttribPointer(sp->a("normal"), 4, GL_FLOAT, false, 0, model.normals.data()); //Wskaż tablicę z danymi dla atrybutu normal

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex);
		glUniform1i(sp->u("textureMap0"), 0);

		//Narysuj obiekt
		glDrawElements(GL_TRIANGLES, model.indices.size(), GL_UNSIGNED_INT, model.indices.data());

		glDisableVertexAttribArray(sp->a("vertex"));  //Wyłącz przesyłanie danych do atrybutu vertex
		glDisableVertexAttribArray(sp->a("normal"));  //Wyłącz przesyłanie danych do atrybutu normal
		glDisableVertexAttribArray(sp->a("texCoord0"));
	}

	glDisableVertexAttribArray(skyBox->a("vertex"));  //Wyłącz przesyłanie danych do atrybutu vertex
	glDisableVertexAttribArray(skyBox->a("texCoord0")); //Wyłącz przesyłanie danych do atrybutu texCoord0

	glfwSwapBuffers(window); //Przerzuć tylny bufor na przedni
}


int main(void)
{
	GLFWwindow* window; //Wskaźnik na obiekt reprezentujący okno

	glfwSetErrorCallback(error_callback);//Zarejestruj procedurę obsługi błędów

	if (!glfwInit()) { //Zainicjuj bibliotekę GLFW
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(width, hight, "Tank Simulator", NULL, NULL);  //Utwórz okno 500x500 o tytule "Tank Simulator" i kontekst OpenGL.

	if (!window) //Jeżeli okna nie udało się utworzyć, to zamknij program
	{
		fprintf(stderr, "Nie można utworzyć okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window); //Od tego momentu kontekst okna staje się aktywny i polecenia OpenGL będą dotyczyć właśnie jego.
	glfwSwapInterval(1); //Czekaj na 1 powrót plamki przed pokazaniem ukrytego bufora

	if (glewInit() != GLEW_OK) { //Zainicjuj bibliotekę GLEW
		fprintf(stderr, "Nie można zainicjować GLEW.\n");
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window); //Operacje inicjujące

	//Główna pętla

	while (!glfwWindowShouldClose(window)) //Tak długo jak okno nie powinno zostać zamknięte
	{
		glfwSetTime(0); //Zeruj timer
		drawScene(window); //Wykonaj procedurę rysującą
		glfwPollEvents(); //Wykonaj procedury callback w zalezności od zdarzeń jakie zaszły.
		processInput(window);
	}

	freeOpenGLProgram(window);

	glfwDestroyWindow(window); //Usuń kontekst OpenGL i okno
	glfwTerminate(); //Zwolnij zasoby zajęte przez GLFW
	exit(EXIT_SUCCESS);
}