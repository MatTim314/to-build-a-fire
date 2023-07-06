 #version 330

    layout (location = 0) in vec3 position;
    //layout (location = 1) in vec2 velocity;

    uniform float time;

    //out vec2 velocityPass;

    void main() {
      //velocityPass = velocity;
      //vec3 newPos = position + velocity * time;
      vec3 newPos = position;
      newPos.y -= 10.0 * time;
      if (newPos.y < 0.0) {
        newPos.y = 10.0;
      }
      gl_Position = vec4(newPos, 1.0);
    }