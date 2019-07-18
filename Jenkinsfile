#!groovy

pipeline {
    agent any

    options {
        buildDiscarder(logRotator(numToKeepStr: '10'))
    }
    
    stages {
        stage ('Clean') {
            steps {
                sh 'make clean'
            }
        }

        stage ('Compile') {
            steps {
                sh 'make compile'
            }
        }
        
        stage ('Build') {
            steps {
                sh 'make zip'
            }
        }

        stage ('Archive') {
            steps {
                //adds <date>-<branch>-<buildnumber> to zip name
                sh '''
                    TODAY=`date +%Y%m%d`
                    TAG=`echo "${TODAY}-${BRANCH_NAME}-${BUILD_NUMBER}" | tr "/" "_"`
                    mv archive.zip hps-${TAG}.zip
                '''
                
                archiveArtifacts artifacts: '*.zip'
            }
        }
    }
}