// Geometry of the regular dodecahedron (inradius 1) and its hyperbolic sibling.
//
// Face normals are the 12 icosahedron vertex directions. Vertices are the
// points shared by three mutually adjacent faces; edges join vertices that
// share two faces.

const PHI = (1 + Math.sqrt(5)) / 2;

function norm(v) {
  const l = Math.hypot(v[0], v[1], v[2]);
  return [v[0] / l, v[1] / l, v[2] / l];
}
const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];

export const FACES = [];
for (const s1 of [-1, 1]) for (const s2 of [-1, 1]) {
  FACES.push(norm([0, s1, s2 * PHI]));
  FACES.push(norm([s1, s2 * PHI, 0]));
  FACES.push(norm([s2 * PHI, 0, s1]));
}

// Solve u_a·x = u_b·x = u_c·x = 1 by Cramer's rule.
function solve3(a, b, c) {
  const cross = (p, q) => [p[1] * q[2] - p[2] * q[1], p[2] * q[0] - p[0] * q[2], p[0] * q[1] - p[1] * q[0]];
  const bc = cross(b, c), ca = cross(c, a), ab = cross(a, b);
  const det = dot(a, bc);
  return [0, 1, 2].map((i) => (bc[i] + ca[i] + ab[i]) / det);
}

const ADJ = 1 / Math.sqrt(5); // cosine between adjacent face normals
const adjacent = (i, j) => Math.abs(dot(FACES[i], FACES[j]) - ADJ) < 1e-6;

export const VERTICES = [];
const vertexFaces = [];
for (let a = 0; a < 12; a++) for (let b = a + 1; b < 12; b++) for (let c = b + 1; c < 12; c++) {
  if (adjacent(a, b) && adjacent(b, c) && adjacent(a, c)) {
    VERTICES.push(solve3(FACES[a], FACES[b], FACES[c]));
    vertexFaces.push([a, b, c]);
  }
}

export const EDGES = [];
for (let i = 0; i < VERTICES.length; i++) for (let j = i + 1; j < VERTICES.length; j++) {
  const shared = vertexFaces[i].filter((f) => vertexFaces[j].includes(f));
  if (shared.length === 2) EDGES.push([VERTICES[i], VERTICES[j]]);
}

// Klein-model inradius for the right-angled dodecahedron of the {5,3,4}
// honeycomb: adjacent faces are orthogonal when u_i·u_j = t², so t = 5^(-1/4).
export const KLEIN_T = Math.pow(5, -0.25);

export const flat = (list) => new Float32Array(list.flat());
