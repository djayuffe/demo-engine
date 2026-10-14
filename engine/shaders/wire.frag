#version 410 core
in float gDepth;
in vec3 gObject;
in vec2 gNdc;
in float gSide;
in float gAlong;
flat in float gEdge;
// shader bodies below use the old names
#define vDepth gDepth
#define vObject gObject
#define vNdc gNdc
uniform float uWidthPx;

out vec4 FragColor;
uniform float uTime;
uniform vec3 uColor;
uniform float uMusicLevel;
uniform float uGain;   // density compensation for additive blending (C++: wireGainFor)
uniform int uStyle;    // 0 = electric palette, 1 = golden glimmer

vec3 palette(float t){
  return .50+.50*cos(6.2831853*(vec3(.03,.31,.62)+t));
}

float lightSheet(vec3 p,vec3 n,float phase,float width){
  float d=dot(p,normalize(n))+sin(phase)*.46;
  return exp(-abs(d)*width);
}

// Signal packets: every wire carries a bright pulse that runs along it at its own speed and phase, so the
// whole mesh reads as circuitry with data flowing through it. Returns an extra gain (>=0) for this fragment.
float packet(float along,float id,float t,float music){
  float h1=fract(sin(id*12.9898)*43758.5453),h2=fract(sin(id*78.233+1.7)*12345.678);
  float speed=.18+.5*h1+.25*music;
  float head=fract(t*speed+h2);
  float d=abs(along-head);d=min(d,1.-d);                       // wrap: the packet re-enters at the other end
  float on=step(.35,h1);                                        // only some wires carry a packet at a time
  return on*exp(-d*d*260.)*(1.6+1.6*music);
}
void main(){
  // soft-edged line profile: full intensity in the core, falling to 0 over the antialiasing skirt
  float hw=.5*uWidthPx+.75;
  float edge=1.-smoothstep((hw-.75)/hw,1.,abs(gSide));
  edge*=1./max(1.,.55*uWidthPx);   // keep total energy near that of a 1 px line as wires get wider
  float music=clamp(uMusicLevel,0.,1.);
  vec3 p=vObject;
  if(uStyle==1){
    // Golden glimmer. Brightness comes from additive overlap (dense orbit regions glow, as in the
    // uber256-lab originals); on top, each little dash twinkles on its own phase and a shimmer band
    // sweeps across the cloud.
    vec3 h=fract(sin(vec3(dot(floor(p*90.),vec3(12.9898,78.233,37.719)))+vec3(0.,1.7,3.1))*vec3(43758.5453,22578.1459,19642.349));
    float twinkle=pow(.5+.5*sin(uTime*(2.6+4.0*h.y)+h.x*6.2831853),30.);
    float sheen=exp(-abs(dot(p,normalize(vec3(.8,.45,.3)))-2.4*sin(uTime*.45))*2.6);
    vec3 gold=mix(vec3(1.0,.50,.10),vec3(1.0,.80,.36),h.z);
    vec3 col=gold*(.80+.75*sheen+.40*music)*(.55+.45*vDepth)+vec3(1.0,.94,.72)*twinkle*5.0;
    FragColor=vec4(col*uGain*2.4*edge,1.0);
    return;
  }

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

  float pk=packet(clamp(gAlong,0.,1.),gEdge,uTime,music);
  color+=palette(wirePhase*.5+uTime*.08+gEdge*.013)*pk*1.1*vDepth;
  FragColor=vec4(color*uGain*edge,1.0);
}
