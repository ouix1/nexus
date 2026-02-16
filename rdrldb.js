
////////////////////////////////////////////////////////////////////////////
// FIREBASE INIT
////////////////////////////////////////////////////////////////////////////

const firebaseConfig = {
  apiKey: "AIzaSyDr5USKStzLZ6OQCfTBbZBmqihhbpBNmCY",
  authDomain: "dsc-dba80.firebaseapp.com",
  projectId: "dsc-dba80",
  storageBucket: "dsc-dba80.firebasestorage.app",
  messagingSenderId: "631849635750",
  appId: "1:631849635750:web:6a43bd500ffef3cced0967",
  databaseURL: "https://dsc-dba80-default-rtdb.firebaseio.com/"
};

firebase.initializeApp(firebaseConfig);
const db = firebase.database();


////////////////////////////////////////////////////////////////////////////
// THREE.JS SETUP
////////////////////////////////////////////////////////////////////////////

const scene = new THREE.Scene();

const camera = new THREE.PerspectiveCamera(
  75,
  window.innerWidth / window.innerHeight,
  0.1,
  2000
);

camera.position.set(200, 200, 200);
camera.lookAt(0, 0, 0);

const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setSize(window.innerWidth, window.innerHeight);
document.body.appendChild(renderer.domElement);

// GRID
const grid = new THREE.GridHelper(400, 40, 0xffffff, 0x444444);
scene.add(grid);

// LIGHT
scene.add(new THREE.HemisphereLight(0xffffff, 0x444444, 1));

const pointMaterial = new THREE.MeshBasicMaterial({ color: 0xff0000 });
const pointsGroup = new THREE.Group();
scene.add(pointsGroup);


////////////////////////////////////////////////////////////////////////////
// FIREBASE LISTENER → POINTS UPDATE
////////////////////////////////////////////////////////////////////////////

db.ref("positions").on("value", snap => {
  const data = snap.val();
  if (!data) return;

  pointsGroup.clear();

  Object.values(data).forEach(p => {
    const sphere = new THREE.Mesh(
      new THREE.SphereGeometry(2, 8, 8),
      pointMaterial
    );
    sphere.position.set(p.x, p.y, p.z);
    pointsGroup.add(sphere);
  });

  console.log("Points updated", pointsGroup.children.length);
});


////////////////////////////////////////////////////////////////////////////
// ANIMATION LOOP
////////////////////////////////////////////////////////////////////////////

function animate() {
  requestAnimationFrame(animate);
  scene.rotation.y += 0.002;
  renderer.render(scene, camera);
}

animate();


////////////////////////////////////////////////////////////////////////////
// RESIZE HANDLER
////////////////////////////////////////////////////////////////////////////

window.addEventListener("resize", () => {
  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(window.innerWidth, window.innerHeight);
});
