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
vec2 projectPointFromHyperbolic(vec3 point) {
	vec3 ret = { 0.0f, 0.0f, -1.0f };
	vec3 vecMultiply = { point.x, point.y, (point.z + 1) };
	ret = ret + (1 / (point.z + 1)) * vecMultiply;
	return { ret.x, ret.y };
}
vec3 getPerpendicular(vec3 point, vec3 v) {
	return cross({ v.x, v.y, (-1.0f * v.z) }, { point.x, point.y, (-1.0f * point.z) });
}
vec3 hyperbolicPointByDistAndDirection(vec3 originPoint, float distance, vec3 vector) {
	float coshT = coshf(distance);
	float sinhT = sinhf(distance);
	return (originPoint * coshT) + (vector * sinhT);
}
vec3 hyperbolicVectorByDistAndDirection(vec3 originPoint, float distance, vec3 vector) {
	float coshT = coshf(distance);
	float sinhT = sinhf(distance);
	return (originPoint * sinhT) + (vector * coshT);
}
float hyperbolicDistanceBetween(vec3 from, vec3 to) {
	return acoshf(dotProduct(-to, from));
}
vec3 hyperbolicAngleBetween(vec3 p, vec3 q) {
	float dist = hyperbolicDistanceBetween(p, q);
	return (q - p * coshf(dist)) / sinhf(dist);
}

vec3 hyperbolicRotateByAngle(vec3 point, vec3 vector, float angle) {
	vec3 perpendicular = getPerpendicular(point, vector);
	vec3 ret = (vector * cosf(angle)) + (perpendicular * sinf(angle));
	return normalizeHyperbolic(ret);
}
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
vec3 moveVector(vec3 toMove, vec3 point, float distance) {
	float coshT = coshf(distance);
	float sinhT = sinhf(distance);
	return normalizeHyperbolic((point * sinhT) + (toMove * coshT));
}
vec3 createVectorOnHyperbola(vec3 p) {
	vec3 v = {
		1,
		2,
		(p.x * 1 + p.y * 2) / p.z
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
vec3 hyperbolicClosestPoint(std::vector<vec3> toCheck, vec3 goal) {
	vec3 closestPoint = toCheck[0];
	for (int i = 0; i < toCheck.size(); i++) {
		if (hyperbolicDistanceBetween(toCheck[i], goal) < hyperbolicDistanceBetween(closestPoint, goal)) {
			closestPoint = toCheck[i];
		}
	}
	return closestPoint;
}


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
	const float bodyRadius = 0.25f;
	std::vector<vec2> body2d;
	std::vector<vec2> eyes2d;
	float direction = 0.0f;
	vec3 hyperbolicEyeCenter1;
	vec3 hyperbolicEyeCenter2;
	const float eyeRadius = 0.075f;
	std::vector<vec3> hyperbolicUfoEyes;
	bool color;
	std::vector<vec2> drool;
	float timeOfLastDraw;
	int drawsSinceLastMouthDraw = 0;

	Ufo() {}
	Ufo(bool co, vec3 center) :hyperbolicCenter(center), color(co) {}

	void drawUfo() {
		// Body
		int location = glGetUniformLocation(gpuProgram.getId(), "color");
		// true: red, false: green
		if (color) {
			glUniform3f(location, 1.0f, 0.0f, 0.0f);
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
		}
		glBufferData(GL_ARRAY_BUFFER,
			sizeof(vec2) * body2d.size(),
			body2d.data(),
			GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, body2d.size());

		glUniform3f(location, 1.0f, 1.0f, 1.0f);
		hyperbolicUfoEyes.clear();
		hyperbolicUfoEyes = createHyperbolicCircle(hyperbolicEyeCenter1, eyeRadius, vector);
		std::vector<vec3> hyperEye2 = createHyperbolicCircle(hyperbolicEyeCenter2, eyeRadius, vector);
		hyperbolicUfoEyes.insert(hyperbolicUfoEyes.end(), hyperEye2.begin(), hyperEye2.end());
		eyes2d.clear();
		for (int i = 0; i < hyperbolicUfoEyes.size(); i++) {
			eyes2d.push_back(projectPointFromHyperbolic(hyperbolicUfoEyes[i]));
		}
		glBufferData(GL_ARRAY_BUFFER,
			sizeof(vec2) * eyes2d.size(),
			eyes2d.data(),
			GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, eyes2d.size());
		timeOfLastDraw = glutGet(GLUT_ELAPSED_TIME) / 1000.f;
		drawsSinceLastMouthDraw++;
	}
	void drawMouth() {
		if (drawsSinceLastMouthDraw < 75) {
			return;
		}
		int location = glGetUniformLocation(gpuProgram.getId(), "color");
		glUniform3f(location, 0.0f, 0.0f, 0.0f);

		vec3 mouthCenter = hyperbolicPointByDistAndDirection(hyperbolicCenter, bodyRadius, vector);
		std::vector<vec3> mouth = createHyperbolicCircle(mouthCenter, eyeRadius * 1.5, vector);
		std::vector<vec2> mouth2d;
		for (int i = 0; i < mouth.size(); i++) {
			mouth2d.push_back(projectPointFromHyperbolic(mouth[i]));
		}

		glBufferData(GL_ARRAY_BUFFER,
			sizeof(vec2) * mouth2d.size(),
			mouth2d.data(),
			GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, mouth2d.size());
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
	void calculateEyes() {
		vector = hyperbolicRotateByAngle(hyperbolicCenter, vector, PI / 3);
		hyperbolicEyeCenter1 = hyperbolicPointByDistAndDirection(hyperbolicCenter,
			bodyRadius * 0.75f, vector);
		hyperbolicEyeCenter1 = pointBackToHyperbola(hyperbolicEyeCenter1);

		vector = hyperbolicRotateByAngle(hyperbolicCenter, vector, -1 * (2*PI / 3));
		hyperbolicEyeCenter2 = hyperbolicPointByDistAndDirection(hyperbolicCenter,
			bodyRadius * 0.75f, vector);
		hyperbolicEyeCenter2 = pointBackToHyperbola(hyperbolicEyeCenter2);

		vector = hyperbolicRotateByAngle(hyperbolicCenter, vector, PI / 3);
	}
};
Ufo red(true, { -1.0f, -1.0f, sqrtf(3.0f)});
Ufo green(false, { 3.0f,0.0f,3.1623f });
std::vector<char> keysPressed;
void drawIris(Ufo looking, Ufo at) {
	
	int location = glGetUniformLocation(gpuProgram.getId(), "color");
	glUniform3f(location, 0.0f, 0.0f, 1.0f);

	// Iris 1
	std::vector<vec3> whiteEyeCircumference1;
	for (int i = 0; i < looking.hyperbolicUfoEyes.size() / 2; i++) {
		if (i % 3 == 0) {
			continue;
		}
		whiteEyeCircumference1.push_back(looking.hyperbolicUfoEyes[i]);
	}
	std::vector<vec3> hyperbolicIrisBody1 = createHyperbolicCircle(
		hyperbolicClosestPoint(whiteEyeCircumference1, at.hyperbolicCenter),
		looking.eyeRadius / 1.5, looking.vector);

	std::vector<vec2> irisbody2d1;
	for (int i = 0; i < hyperbolicIrisBody1.size(); i++) {
		irisbody2d1.push_back(projectPointFromHyperbolic(hyperbolicIrisBody1[i]));
	}

	glBufferData(GL_ARRAY_BUFFER,
		sizeof(vec2) * irisbody2d1.size(),
		irisbody2d1.data(),
		GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, irisbody2d1.size());

	// Iris 2
	
	std::vector<vec3> whiteEyeCircumference2;
	for (int i = looking.hyperbolicUfoEyes.size() / 2; i < looking.hyperbolicUfoEyes.size(); i++) {
		if (i % 3 == 0) {
			continue;
		}
		whiteEyeCircumference2.push_back(looking.hyperbolicUfoEyes[i]);
	}

	std::vector<vec3> hyperbolicIrisBody2 = createHyperbolicCircle(
		hyperbolicClosestPoint(whiteEyeCircumference2, at.hyperbolicCenter),
		looking.eyeRadius / 1.5, looking.vector);

	std::vector<vec2> irisbody2d2;
	for (int i = 0; i < hyperbolicIrisBody2.size(); i++) {
		irisbody2d2.push_back(projectPointFromHyperbolic(hyperbolicIrisBody2[i]));
	}

	glBufferData(GL_ARRAY_BUFFER,
		sizeof(vec2) * irisbody2d2.size(),
		irisbody2d2.data(),
		GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, irisbody2d2.size());
	
}


// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	glGenVertexArrays(1, &vao);	// get 1 vao id
	glBindVertexArray(vao);		// make it active

	red.vector = createVectorOnHyperbola(red.hyperbolicCenter);
	green.vector = createVectorOnHyperbola(green.hyperbolicCenter);
	red.calculateEyes(); green.calculateEyes();
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


	if (contains(keysPressed, 'e')) {
		vec2 drool = projectPointFromHyperbolic(red.hyperbolicCenter);
		red.drool.push_back(drool);
		red.hyperbolicCenter = hyperbolicPointByDistAndDirection(red.hyperbolicCenter, 0.05f,
			red.vector);
		red.vector = moveVector(red.vector, red.hyperbolicCenter, 0.05f);
		red.calculateEyes();
		red.hyperbolicCenter = pointBackToHyperbola(red.hyperbolicCenter);
		drool = projectPointFromHyperbolic(red.hyperbolicCenter);
		red.drool.push_back(drool);
	}
	if (contains(keysPressed, 'f')) {
		red.direction = PI / 50;
		red.vector = hyperbolicRotateByAngle(red.hyperbolicCenter, red.vector, red.direction);
		red.calculateEyes();
	}
	if (contains(keysPressed, 's')) {
		red.direction = -1 * (PI / 50);
		red.vector = hyperbolicRotateByAngle(red.hyperbolicCenter, red.vector, red.direction);
		red.calculateEyes();
	}
	green.drawUfo();
	red.drawUfo();
	red.drawDrool();
	drawIris(green, red); drawIris(red, green);
	green.drawDrool();
	red.drawMouth(); green.drawMouth();

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
	if ((time - green.timeOfLastDraw) > 0.02) {
		vec2 drool = projectPointFromHyperbolic(green.hyperbolicCenter);
		green.drool.push_back(drool);

		green.direction = PI / 25;
		green.vector = hyperbolicRotateByAngle(green.hyperbolicCenter, green.vector, green.direction);
		green.calculateEyes();

		green.hyperbolicCenter = hyperbolicPointByDistAndDirection(green.hyperbolicCenter, 0.05f,
			green.vector);
		green.vector = moveVector(green.vector, green.hyperbolicCenter, 0.05f);
		green.calculateEyes();
		green.hyperbolicCenter = pointBackToHyperbola(green.hyperbolicCenter);
		drool = projectPointFromHyperbolic(green.hyperbolicCenter);
		green.drool.push_back(drool);
		glutPostRedisplay();
	}
}