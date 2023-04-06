//=============================================================================================
// Mintaprogram: Zld hromszg. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Krafcsik Gergo
// Neptun : PAI2R6
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"

// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char* const vertexSource = R"(
	#version 330				// Shader 3.3
	precision highp float;		// normal floats, makes no difference on desktop computers

	uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
	layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0

	void main() {
		gl_Position = vec4(vp.x, vp.y, 0, 1) * MVP;		// transform vp from modeling space to normalized device space
	}
)";

// fragment shader in GLSL
const char* const fragmentSource = R"(
	#version 330			// Shader 3.3
	precision highp float;	// normal floats, makes no difference on desktop computers
	
	uniform vec3 color;		// uniform variable, the color of the primitive
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		outColor = vec4(color, 1);	// computed color is the color of the primitive
	}
)";

GPUProgram gpuProgram; // vertex and fragment shaders
unsigned int vao;	   // virtual world on the GPU
int trianglesInCircle = 100;
float PI = static_cast<float>(M_PI);
int i = 0;


float dotProduct(vec3 p, vec3 q) {
	return p.x * q.x + p.y * q.y - p.z * q.z;
}
vec3 normalizeHyperbolic(vec3 v) {
	return v / sqrtf(dotProduct(v, v));
}

// vetites hiperbolikusrol 2d diszkre
vec2 projectPointFromHyperbolic(vec3 point) {
	vec3 ret = { 0.0f, 0.0f, -1.0f };
	vec3 vecMultiply = { point.x, point.y, (point.z + 1) };
	ret = ret + (1 / (point.z + 1)) * vecMultiply;
	return { ret.x, ret.y };
}

// 1. iranyra meroleges
vec3 getPerpendicular(vec3 point, vec3 v) {
	return cross({ v.x, v.y, (-1.0f * v.z) }, { point.x, point.y, (-1.0f * point.z) });
}
// 2. pont helyenek szamitasa t idovel kesobb
vec3 hyperbolicPointByDistAndDirection(vec3 originPoint, float distance, vec3 vector) {
	float coshT = coshf(distance);
	float sinhT = sinhf(distance);
	return (originPoint * coshT) + (vector * sinhT);
}
// 2/2. pont sebessegvektoranak szamitasa t idovel kesobb
vec3 hyperbolicVectorByDistAndDirection(vec3 originPoint, float distance, vec3 vector) {
	float coshT = coshf(distance);
	float sinhT = sinhf(distance);
	return (originPoint * sinhT) + (vector * coshT);
}
// 3/1. egy ponthoz kepest egy masik pont tavolsaganak meghatarozasa
float hyperbolicDistanceBetween(vec3 from, vec3 to) {
	return acoshf(dotProduct(-to, from));
}
// 3/2. egy ponthoz kepest egy masik pont iranyanak meghatarozasa
vec3 hyperbolicAngleBetween(vec3 p, vec3 q) {
	float dist = hyperbolicDistanceBetween(p, q);
	return (q - p * coshf(dist)) / sinhf(dist);
}
// 4. egy ponthoz kepest adott iranyba es tavolsagra levo pont eloallitasa

// 5. egy pontban egy vektor elforgatasa adott szoggel
vec3 hyperbolicRotateByAngle(vec3 point, vec3 vector, float angle) {
	vec3 perpendicular = getPerpendicular(point, vector);
	vec3 ret = (vector * cosf(angle)) + (perpendicular * sinf(angle));
	return normalizeHyperbolic(ret);
}
// 6. rajta legyunk a hiperbolan
vec3 pointBackToHyperbola(vec3 p) {
	return {
		p.x,
		p.y,
		sqrtf((p.x * p.x) + (p.y * p.y) + 1)
	};
}
bool checkIfPointOnHyperbola(vec3 p) {
	return (p.x * p.x) + (p.y * p.y) - (p.z * p.z) == -1;
}
// sebessegvektor eltolasa mozgas utan
vec3 moveVector(vec3 toMove, vec3 point, float distance) {
	float coshT = coshf(distance);
	float sinhT = sinhf(distance);
	return normalizeHyperbolic((point * sinhT) + (toMove * coshT));
}
vec3 createVectorOnHyperbola(vec3 p) {
	vec3 v = {
		1,
		2,
		(p.x * 1 + p.y * 1) / p.z
	};
	return normalizeHyperbolic(v);
}

std::vector<vec3> createHyperbolicCircle(vec3 center, float radius, vec3 vector) {
	const float angle = 2 * PI / trianglesInCircle;
	std::vector<vec3> circle;
	for (int i = 0; i < trianglesInCircle; i++) {
		circle.push_back(center);
		circle.push_back(
			hyperbolicPointByDistAndDirection(center, radius,
				hyperbolicRotateByAngle(center, vector, angle * i)));
		circle.push_back(
			hyperbolicPointByDistAndDirection(center, radius,
				hyperbolicRotateByAngle(center, vector, angle * (i + 1))));
	}
	return circle;
}

/////////////////////////////////////////////////////////

bool floatEquals(float a, float b) {
	float epsilon = 0.0000000f;
	float difference = (a - b) < 0 ? -(a - b) : (a - b);
	return difference < epsilon;
}
vec2 rotateByAngle(vec2 around, vec2 toRotate, float angle) {
	vec2 ret = { cosf(angle) * (toRotate.x - around.x) - sinf(angle) * (toRotate.y - around.y)
		+ around.x,
		sinf(angle) * (toRotate.x - around.x) + cosf(angle) * (toRotate.y - around.y)
		+ around.y
	};
	return ret;
}
vec2 pointByDistAndDirection(vec2 origin, float distance, float angle) {
	return { origin.x + distance * cosf(angle),
			origin.y + distance * sinf(angle) };
}
std::vector<vec2> createCircle(vec3 oldCenter, float radius) {
	const float angle = 2 * PI / trianglesInCircle;
	std::vector<vec2> circle;
	vec2 center = projectPointFromHyperbolic(oldCenter);
	for (int i = 0; i < trianglesInCircle; i++) {
		circle.push_back(center);
		circle.push_back(pointByDistAndDirection(center, radius, angle * i));
		circle.push_back(pointByDistAndDirection(center, radius, angle * (i + 1)));
	}
	return circle;
}
float getAngle(vec2 from, vec2 to) {
	return acos(dotProduct(from, to) / (length(from) * length(to)));
}
float distanceBetween(vec2 from, vec2 to) {
	float dx = to.x - from.x;
	float dy = to.y - from.y;
	return sqrt(dx * dx + dy * dy);
}
vec2 getClosestPoint(std::vector<vec2> toCheck, vec2 goal) {
	vec2 closestPoint = toCheck[0];
	for (int i = 0; i < toCheck.size(); i++) {
		if (distanceBetween(toCheck[i], goal) < distanceBetween(closestPoint, goal)) {
			closestPoint = toCheck[i];
		}
	}
	return closestPoint;
}
bool contains(std::vector<char> in, char toCheck) {
	for (int i = 0; i < in.size(); i++) {
		if (in[i] == toCheck) {
			return true;
		}
	}
	return false;
}

struct Ufo {
	vec3 hyperbolicCenter;
	vec3 vector;
	std::vector<vec3> hyperbolicBody;

	std::vector<vec2> body2d;

	float direction = 0.0f;

	vec2 eyeCenter1;
	vec2 eyeCenter2;
	bool color;
	std::vector<vec2> drool;
	float timeOfLastDraw;
	const float bodyRadius = 0.25f;
	const float eyeRadius = 0.025f;
	std::vector<vec2> ufoEyes;
	int drawsSinceLastMouthDraw = 0;

	Ufo() {}
	Ufo(bool co, vec3 center) :hyperbolicCenter(center), color(co) {}

	void drawUfo() {
		// Body
		int location = glGetUniformLocation(gpuProgram.getId(), "color");
		// true: red, false: green
		if (color) {
			glUniform3f(location, 1.0f, 0.0f, 0.0f);
			printf("hyperbolicCenter: X: %f, Y: %f, Z: %f\n", hyperbolicCenter.x, hyperbolicCenter.y, hyperbolicCenter.z);
		}
		else {
			glUniform3f(location, 0.0f, 1.0f, 0.0f);
		}
		hyperbolicBody.clear();
		vec2 center = projectPointFromHyperbolic(hyperbolicCenter);

		hyperbolicBody = createHyperbolicCircle(hyperbolicCenter, bodyRadius, vector);

		body2d.clear();
		for (int i = 0; i < hyperbolicBody.size(); i++) {
			body2d.push_back(projectPointFromHyperbolic(hyperbolicBody[i]));
			//printf("Hyperbolic[%d]: X: %f, Y: %f, Z: %f\n2D[%d]: X: %f, Y: %f\n", i, hyperbolicBody[i].x, hyperbolicBody[i].y, hyperbolicBody[i].z, i, body2d[i].x, body2d[i].y);
		}

		glBufferData(GL_ARRAY_BUFFER,
			sizeof(vec2) * body2d.size(),
			body2d.data(),
			GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, body2d.size());

		// White eyes
		/*
		glUniform3f(location, 1.0f, 1.0f, 1.0f);
		ufoEyes.clear();
		vec2 eyeCenter1Temp = pointByDistAndDirection(center, 0.075f, (PI / 3 + direction));
		vec3 eyeCenter1 = {
			eyeCenter1Temp.x,
			eyeCenter1Temp.y,
			0.0f
		};
		ufoEyes = createCircle(eyeCenter1, eyeRadius);

		vec2 eyeCenter2Temp = pointByDistAndDirection(center, 0.075f, (5 * PI / 3 + direction));
		vec3 eyeCenter2 = {
			eyeCenter2Temp.x,
			eyeCenter2Temp.y,
			0.0f
		};
		std::vector<vec2> eye2 = createCircle(eyeCenter2, eyeRadius);
		ufoEyes.insert(ufoEyes.end(), eye2.begin(), eye2.end());

		glBufferData(GL_ARRAY_BUFFER,
			sizeof(vec2) * ufoEyes.size(),
			ufoEyes.data(),
			GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, ufoEyes.size());
		timeOfLastDraw = glutGet(GLUT_ELAPSED_TIME) / 1000.f;
		drawsSinceLastMouthDraw++;
		*/
	}
	void drawMouth() {
		if (drawsSinceLastMouthDraw < 75) {
			return;
		}
		int location = glGetUniformLocation(gpuProgram.getId(), "color");
		glUniform3f(location, 0.0f, 0.0f, 0.0f);

		vec2 center = projectPointFromHyperbolic(hyperbolicCenter);
		vec2 mouthCenterTemp = pointByDistAndDirection(center, bodyRadius, direction);
		vec3 mouthCenter = {
			mouthCenterTemp.x,
			mouthCenterTemp.y,
			0.0f
		};
		std::vector<vec2> mouth = createCircle(mouthCenter, eyeRadius * 2);

		glBufferData(GL_ARRAY_BUFFER,
			sizeof(vec2) * mouth.size(),
			mouth.data(),
			GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, mouth.size());
		if (drawsSinceLastMouthDraw > 150) {
			drawsSinceLastMouthDraw = 0;
		}
	}
	void drawDrool() {
		int location = glGetUniformLocation(gpuProgram.getId(), "color");
		glUniform3f(location, 1.0f, 1.0f, 1.0f);
		glBufferData(GL_ARRAY_BUFFER,
			sizeof(vec2) * drool.size(),
			drool.data(),
			GL_DYNAMIC_DRAW);
		glDrawArrays(GL_LINES, 0, drool.size());
	}
};

Ufo red(true, { 0.0f, 0.0f, 1.0f });
Ufo green(false, { 3.0f,0.0f,3.1623f });
std::vector<char> keysPressed;

void drawIris(Ufo looking, Ufo at) {
	int location = glGetUniformLocation(gpuProgram.getId(), "color");
	glUniform3f(location, 0.0f, 0.0f, 1.0f);

	std::vector<vec2> irisPath1;
	for (int i = 0; i < looking.ufoEyes.size() / 2; i++) {
		if (i % 3 == 0) {
			continue;
		}
		irisPath1.push_back(looking.ufoEyes[i]);
	}
	vec2 atCenter = projectPointFromHyperbolic(at.hyperbolicCenter);


	std::vector<vec2> irisBody1 = createCircle(
		getClosestPoint(irisPath1, atCenter),
		looking.eyeRadius / 1.5);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(vec2) * irisBody1.size(),
		irisBody1.data(),
		GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, irisBody1.size());

	std::vector<vec2> irisPath2;
	for (int i = looking.ufoEyes.size() / 2; i < looking.ufoEyes.size(); i++) {
		if (i % 3 == 0) {
			continue;
		}
		irisPath2.push_back(looking.ufoEyes[i]);
	}

	std::vector<vec2> irisBody2 = createCircle(
		getClosestPoint(irisPath2, atCenter),
		looking.eyeRadius / 1.5);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(vec2) * irisBody2.size(),
		irisBody2.data(),
		GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, irisBody2.size());
}
// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	glGenVertexArrays(1, &vao);	// get 1 vao id
	glBindVertexArray(vao);		// make it active

	red.vector = (createVectorOnHyperbola(red.hyperbolicCenter));
	green.vector = (createVectorOnHyperbola(green.hyperbolicCenter));
	// create program for the GPU
	gpuProgram.create(vertexSource, fragmentSource, "outColor");
}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0.25f, 0.25f, 0.25f, 0);     // background color
	glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer

	int location = glGetUniformLocation(gpuProgram.getId(), "color");
	glUniform3f(location, 0.0f, 0.0f, 0.0f); // 3 floats

	float MVPtransf[4][4] = { 1, 0, 0, 0,    // MVP matrix, 
							  0, 1, 0, 0,    // row-major!
							  0, 0, 1, 0,
							  0, 0, 0, 1 };
	location = glGetUniformLocation(gpuProgram.getId(), "MVP");	// Get the GPU location of uniform variable MVP
	glUniformMatrix4fv(location, 1, GL_TRUE, &MVPtransf[0][0]);	// Load a 4x4 row-major float matrix to the specified location

	unsigned int vbo;	   // vertex buffer object
	glGenBuffers(1, &vbo);	// Generate 1 buffer
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	vec3 center = { 0.0f, 0.0f, 0.0f };
	std::vector<vec2> circle = createCircle(center, 1.0f);

	glBufferData(GL_ARRAY_BUFFER, 	// Copy to GPU target
		sizeof(vec2) * circle.size(),  // # bytes
		circle.data(),	      	// address
		GL_DYNAMIC_DRAW);	// we do not change later

	glEnableVertexAttribArray(0);  // AttribArray 0
	glVertexAttribPointer(0,       // vbo -> AttribArray 0
		2, GL_FLOAT, GL_FALSE, // two floats/attrib, not fixed-point
		0, NULL); 		     // stride, offset: tightly packed

	glBindVertexArray(vao);  // Draw call
	glDrawArrays(GL_TRIANGLES, 0 /*startIdx*/, circle.size() /*# Elements*/);
	printf("red vector: X: %f, Y: %f, Z: %f\n", red.vector.x, red.vector.y, red.vector.z);
	if (contains(keysPressed, 'e')) {
		vec2 drool = projectPointFromHyperbolic(red.hyperbolicCenter);
		red.drool.push_back(drool);
		red.hyperbolicCenter = hyperbolicPointByDistAndDirection(red.hyperbolicCenter, 0.05f,
			red.vector);
		red.hyperbolicCenter = pointBackToHyperbola(red.hyperbolicCenter);
		red.vector = moveVector(red.vector, red.hyperbolicCenter, 0.05f);
		drool = projectPointFromHyperbolic(red.hyperbolicCenter);
		red.drool.push_back(drool);
	}
	if (contains(keysPressed, 'f')) {
	//for (int i = 0; i < 1000; i++) {
		red.direction += PI / 100;
		red.vector = hyperbolicRotateByAngle(red.hyperbolicCenter, red.vector, red.direction);
		printf("[]. forgatas: Red vector: X: %f, Y: %f, Z: %f\n", red.vector.x, red.vector.y, red.vector.z);
	//}
	}
	if (contains(keysPressed, 's')) {
		red.direction -= PI / 100;
		red.vector = hyperbolicRotateByAngle(red.hyperbolicCenter, red.vector, red.direction);
	}
	printf("red vector: X: %f, Y: %f, Z: %f\n", red.vector.x, red.vector.y, red.vector.z);
	printf("red hyperCenter * red hyperCenter: %f == -1 ??????????\n", dotProduct(red.hyperbolicCenter, red.hyperbolicCenter));
	printf("red hyperCenter * red vector: %f == 0 ??????????\n", dotProduct(red.hyperbolicCenter, red.vector));
	/*
	printf("Red hyperbolicCenter X = %f, Y = %f, Z = %f \n",
		red.hyperbolicCenter.x, red.hyperbolicCenter.y, red.hyperbolicCenter.z);

	printf("Red center: X: %f, Y: %f, Z: %f\n"
		"Green center: X: %f, Y: %f, Z: %f\n"
		"Red vector: X: %f, Y: %f, Z: %f\n"
		"Green vector: X: %f, Y: %f, Z: %f\n",
		red.hyperbolicCenter.x, red.hyperbolicCenter.y, red.hyperbolicCenter.z,
		green.hyperbolicCenter.x, green.hyperbolicCenter.y, green.hyperbolicCenter.z,
		red.vector.x, red.vector.y, red.vector.z, green.vector.x, green.vector.y, green.vector.z);

	printf("Green center * Green center: %f == -1 ??\n",
		dotProduct(green.hyperbolicCenter, green.hyperbolicCenter));
	*/
	green.drawUfo();
	red.drawUfo();

	red.drawDrool();
	/*
	green.drawDrool();
	drawIris(green, red); drawIris(red, green);
	red.drawMouth(); green.drawMouth();
	*/
	glDeleteBuffers(1, &vbo);
	glutSwapBuffers(); // exchange buffers for double buffering
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	switch (key) {
	case 'e':
		keysPressed.push_back(key);
		break;
	case 's':
		keysPressed.push_back(key);
		break;
	case 'f':
		keysPressed.push_back(key);
		break;
	default: break;
	}
	glutPostRedisplay();         // if d, invalidate display, i.e. redraw
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
	switch (key) {
	case 'e':
		for (auto it = keysPressed.begin(); it != keysPressed.end(); ) {
			if (*it == key) {
				it = keysPressed.erase(it);
			}
			else {
				++it;
			}
		}
		break;
	case 's':
		for (auto it = keysPressed.begin(); it != keysPressed.end(); ) {
			if (*it == key) {
				it = keysPressed.erase(it);
			}
			else {
				++it;
			}
		}
		break;
	case 'f':
		for (auto it = keysPressed.begin(); it != keysPressed.end(); ) {
			if (*it == key) {
				it = keysPressed.erase(it);
			}
			else {
				++it;
			}
		}
		break;
	default: break;
	}
	glutPostRedisplay();         // if d, invalidate display, i.e. redraw

}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;
	printf("Mouse moved to (%3.2f, %3.2f)\n", cX, cY);

}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	float time = glutGet(GLUT_ELAPSED_TIME) / 1000.f; // elapsed time since the start of the program
	/*
	if ((time - green.timeOfLastDraw) > 0.01) {
		vec2 drool = projectPointFromHyperbolic(green.hyperbolicCenter);
		green.drool.push_back(drool);
		green.direction -= PI / 100;
		green.hyperbolicCenter = hyperbolicPointByDistAndDirection(
			green.hyperbolicCenter, (0.15f * PI) / 100,
			createVector(green.hyperbolicCenter, false));
		drool = projectPointFromHyperbolic(green.hyperbolicCenter);
		green.drool.push_back(drool);
		glutPostRedisplay();
	}
	*/
}