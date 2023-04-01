//=============================================================================================
// Mintaprogram: Zöld háromszög. Ervenyes 2019. osztol.
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

bool floatEquals(float a, float b) {
	float epsilon = std::numeric_limits<float>::epsilon();
	return std::abs(a - b) < epsilon;
}
float dotProduct(vec3 p, vec3 q) {
	return p.x * q.x + p.y * q.y - p.z * q.z;
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
std::vector<vec2> createCircle(vec2 center, float radius) {
	const float angle = 2 * PI / trianglesInCircle;
	std::vector<vec2> circle;

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
bool checkKeyPressed(char toCheck) {
	return (GetAsyncKeyState(toCheck) & 0x8000);
}

struct Ufo {
	std::vector<vec2> body;
	float direction = 0.0f;
	vec2 center;
	bool color;
	std::vector<vec2> drool;

	Ufo(bool co, vec2 ce) {
		color = co;
		center = ce;
	}

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
		body = createCircle(center, 0.1f);

		glBufferData(GL_ARRAY_BUFFER,
			sizeof(vec2) * body.size(),
			body.data(),
			GL_STATIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, body.size());

		// White eyes

		glUniform3f(location, 1.0f, 1.0f, 1.0f);
		vec2 eyeCenter1 = pointByDistAndDirection(center, 0.075f, (PI / 4 + direction));
		std::vector<vec2> ufoEyes = createCircle(eyeCenter1, 0.025f);

		vec2 eyeCenter2 = pointByDistAndDirection(center, 0.075f, (7 * PI / 4 + direction));
		std::vector<vec2> eye2 = createCircle(eyeCenter2, 0.025f);
		ufoEyes.insert(ufoEyes.end(), eye2.begin(), eye2.end());

		glBufferData(GL_ARRAY_BUFFER,
			sizeof(vec2) * ufoEyes.size(),
			ufoEyes.data(),
			GL_STATIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, ufoEyes.size());
	}

	void drawDrool() {
		int location = glGetUniformLocation(gpuProgram.getId(), "color");
		glUniform3f(location, 1.0f, 1.0f, 1.0f);
		glBufferData(GL_ARRAY_BUFFER,
			sizeof(vec2) * drool.size(),
			drool.data(),
			GL_STATIC_DRAW);
		glDrawArrays(GL_LINES, 0, drool.size());
	}
};
Ufo red(true, { -0.7f, 0.3f });
Ufo green(false, { 0.7f, 0.0f });


// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	glGenVertexArrays(1, &vao);	// get 1 vao id
	glBindVertexArray(vao);		// make it active

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

	// disc area

	vec2 center = { 0.0f, 0.0f };
	std::vector<vec2> circle = createCircle(center, 1.0f);

	glBufferData(GL_ARRAY_BUFFER, 	// Copy to GPU target
		sizeof(vec2) * circle.size(),  // # bytes
		circle.data(),	      	// address
		GL_STATIC_DRAW);	// we do not change later

	glEnableVertexAttribArray(0);  // AttribArray 0
	glVertexAttribPointer(0,       // vbo -> AttribArray 0
		2, GL_FLOAT, GL_FALSE, // two floats/attrib, not fixed-point
		0, NULL); 		     // stride, offset: tightly packed

	glBindVertexArray(vao);  // Draw call
	glDrawArrays(GL_TRIANGLES, 0 /*startIdx*/, circle.size() /*# Elements*/);


	green.drawUfo();
	red.drawUfo();

	red.drawDrool();
	/*
	glUniform3f(location, 0.0f, 0.0f, 1.0f);
	vec2 greenIrisCenter1 = pointByDistAndDirection(greenEyeCenter1, 0.01875f,
		getAngle(greenEyeCenter1, centerRedUfo));
	std::vector<vec2> irisis = createCircle(greenIrisCenter1, 0.0125);
	*/

	glutSwapBuffers(); // exchange buffers for double buffering
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	red.drool.push_back(red.center);
	switch (key) {
	case('w'):
		red.center = pointByDistAndDirection(red.center, 0.02f, red.direction);
		break;
	case ('a'):
		if (checkKeyPressed('w')) {
			red.center = pointByDistAndDirection(red.center, 0.02f, red.direction);
		}
		red.direction += 0.08f;
		break;
	case('d'):
		if (checkKeyPressed('w')) {
			red.center = pointByDistAndDirection(red.center, 0.02f, red.direction);
		}
		red.direction -= 0.08f;
		break;
	default:
		break;
	}
	red.drool.push_back(red.center);
	glutPostRedisplay();         // if d, invalidate display, i.e. redraw
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
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

	char* buttonStat;
	switch (state) {
	case GLUT_DOWN: buttonStat = "pressed"; break;
	case GLUT_UP:   buttonStat = "released"; break;
	}

	switch (button) {
	case GLUT_LEFT_BUTTON:   printf("Left button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);   break;
	case GLUT_MIDDLE_BUTTON: printf("Middle button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY); break;
	case GLUT_RIGHT_BUTTON:  printf("Right button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);  break;
	}
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
}