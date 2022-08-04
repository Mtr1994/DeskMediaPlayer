#!/bin/bash
# create by mtr at 2021/05/13 09:59
# 

echo -e "init git repository \r\n"
git init

echo -e "add remote origin \r\n"
git remote add origin git@github.com:Mtr1994/Qt-UI-App-Template.git

echo -e "get template code from master \r\n"
git pull origin master

echo -e "delete git repository ..."
rm -rf .git
rm -rf README.md
rm -rf LICENSE
rm -rf .gitignore
echo -e "finished"