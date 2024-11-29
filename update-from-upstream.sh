#!/bin/bash

set -e 

git checkout upstream
git remote update upstream
git pull
git checkout master
git rebase -i upstream
git push origin master upstream
