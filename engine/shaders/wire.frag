#version 410 core
in float vDepth;
in vec3 vObject;
in vec2 vNdc;

out vec4 FragColor;
uniform float uTime;
uniform vec3 uColor;
uniform float uMusicLevel;
uniform float uGain;   // density compensation for additive blending (C++: wireGainFor)

vec3 palette(float t){
  return .50+.50*cos(6.2831853*(vec3(.03,.31,.62)+t));
}

float lightSheet(vec3 p,vec3 n,float phase,float width){
  float d=dot(p,normalize(n))+sin(phase)*.46;
  return exp(-abs(d)*width);
}

void main(){
  float music=clamp(uMusicLevel,0.,1.);
  vec3 p=vObject;

  float wirePhase=dot(p,vec3(.19,.37,.53))*1.75+length(p.xy)*.23+uTime*.135;
  vec3 wireCycle=palette(wirePhase+music*.12);

  float matrixA=pow(.5+.5*sin(p.x*7.0+p.y*11.0+p.z*13.0+uTime*(2.0+music*1.7)),5.0);
  float matrixB=pow(.5+.5*sin(p.x*17.0-p.y*5.0+p.z*9.0-uTime*(1.35+music)),7.0);
  float matrixC=pow(.5+.5*sin((p.x+p.y-p.z)*21.0+uTime*3.1),12.0);

  float sheet=0.0;
  sheet+=lightSheet(p,vec3(.8,.25,.52),uTime*.82+p.z*.7,7.0);
  sheet+=lightSheet(p,vec3(-.35,.9,.18),uTime*1.15+p.x*.6,9.0);
  sheet+=lightSheet(p,vec3(.18,-.44,1.0),uTime*.67+p.y*.8,6.0);

  float rim=pow(clamp(1.0-abs(vNdc.x)*.36-abs(vNdc.y)*.25,0.,1.),.55);
  float pulse=.82+.16*sin(uTime*1.7+dot(p,vec3(.7,.2,.4)))+music*.55;
  float lattice=matrixA*.95+matrixB*.65+matrixC*.55+sheet*.78;

  vec3 hot=palette(wirePhase*.62+uTime*.05+matrixA*.18);
  vec3 electric=wireCycle*(1.15+lattice*1.35)+hot*(matrixC*.85+sheet*.92);
  vec3 base=uColor*(.22+.16*music);
  vec3 color=(base+electric)*pulse*vDepth*(.62+rim*.55);
  color+=vec3(.12,.34,1.0)*matrixB*music*.75;

  FragColor=vec4(color*uGain,1.0);
}
