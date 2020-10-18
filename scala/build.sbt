name := "zipdu"

ThisBuild / scalaVersion := "2.12.8"
ThisBuild / version      := "0.0.1"
ThisBuild / organization := "io.shiftleft"

enablePlugins(JavaAppPackaging)

maintainer := "Claudiu-Vlad Ursache <claudiu@shiftleft.io>"

lazy val root = (project in file("."))
  .settings(
    libraryDependencies += "com.sparkjava" % "spark-core" % "2.9.3",
    libraryDependencies += "com.google.code.gson" % "gson" % "2.8.6"
  )

