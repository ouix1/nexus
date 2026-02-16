  // Fade-in on scroll


  document.addEventListener('DOMContentLoaded', () => {
    const faders = document.querySelectorAll('.fade-in');

    const appearOptions = {
      threshold: 0.2
    };

    const appearOnScroll = new IntersectionObserver((entries, observer) => {
      entries.forEach(entry => {
        if (!entry.isIntersecting) return;
        entry.target.classList.add('visible');
        observer.unobserve(entry.target);
      });
    }, appearOptions);

    faders.forEach(fader => appearOnScroll.observe(fader));
  });


  //initialize firebase

  // Initialize Firebase using your project's configuration object
app = firebase.initializeApp(firebaseConfig);

// Log the initialized Firebase app (helps for debugging)
console.log(app);

function logIn() {
  // Get the email entered in the login form
  const email = document.getElementById('user_email').value;
  
  // Get the password entered in the login form
  const password = document.getElementById('user_pass').value;

  // Sign in the user with email + password using Firebase Authentication
  firebase.auth().signInWithEmailAndPassword(email, password)
}

function logout() {
  // Log out the current Firebase authenticated user
  firebase.auth().signOut();
}

function signUp() {
  // Get the email entered in the sign-up form
  const email = document.getElementById('user_s_email').value;

  // Get the password entered in the sign-up form
  const password = document.getElementById('user_s_pass').value;

  // Create a new Firebase user account with email + password
  firebase.auth().createUserWithEmailAndPassword(email, password)
}

// -----------------------------
//     REAL-TIME DATABASE
// -----------------------------

// Listen to changes in the "/positions" node of the database in real time
firebase.database().ref("/positions").on("value", snap => {
  // Print the current value of "/positions" whenever it updates
  console.log(snap.val());
});

// Read the entire root "/" of the database just once
firebase.database().ref('/').once("value").then(snap => {
  // Log the full root data, helpful for debugging structure
  console.log("ROOT:", snap.val());
});

// -----------------------------
//   AUTHENTICATION LISTENER
// -----------------------------

// This runs every time the authentication state changes
// (user logs in, logs out, signs up, refreshes page)
firebase.auth().onAuthStateChanged(user => {
  if (user) {
    // If a user is logged in
    console.log("LOGGED IN");

    // Hide login form
    document.getElementById("logindiv").style.display = "none";

    // Show logout button
    document.getElementById("logoutBtn").classList.remove("d-none");

    // Show the start section of the UI
    document.getElementById("startdiv").classList.remove("d-none");

    // Move the avatar to the right
    document.getElementById("avatar").classList.add("ms-auto");
  } else {
    // If user is NOT logged in
    console.log("NOT LOGGED IN");

    // Show login form
    document.getElementById("logindiv").style.display = "block";

    // Hide logout button
    document.getElementById("logoutBtn").classList.add("d-none");

    // Hide the start section (user cannot see it when not logged in)
    document.getElementById("startdiv").classList.add("d-none");

    // Remove alignment on avatar
    document.getElementById("avatar").classList.remove("ms-auto");
  }
});
