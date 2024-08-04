import domain from './constants';
import $ from 'jquery';

const base64encode = (str) => {
    return btoa(unescape(encodeURIComponent(str)));
}

const sample = [
    {
        id: 0,
        from: "Maxime Preaux",
        address: "maxime@codepen.io",
        time: "2022-10-07 15:35:14",
        message:
            "This is my first attempt at using React.\nDuis cursus, diam at pretium aliquet, metus urna convallis erat, eget tincidunt dui augue eu tellus. Phasellus elit pede, malesuada vel, venenatis vel, faucibus id, libero. Donec consectetuer mauris id sapien. Cras",
        subject: "Messing with React.js",
        tag: "inbox",
        read: "false",
    },
    {
        id: 1,
        from: "Dribbble",
        address: "digest@dribbble.com",
        time: "2022-05-09 14:23:54",
        message:
            "Here are the latest shots from Dribbblers you follow! Nec mauris blandit mattis. Cras eget nisi dictum augue malesuada malesuada. Integer id magna et ipsum cursus vestibulum. Mauris magna. Duis dignissim tempor arcu. Vestibulum ut eros non enim commodo hendrerit. Donec porttitor tellus non magna. Nam ligula elit, pretium et, rutrum non, hendrerit id, ante. Nunc mauris sapien,",
        subject: "Dribbble Digest",
        tag: "inbox",
        read: "false",
    },
    {
        id: 2,
        from: "Christopher Medina",
        address: "dolor@luctusutpellentesque.net",
        time: "2021-07-03 21:48:27",
        message:
            "Woops, that last pull request messed up the csproj. Mauris. Morbi non sapien molestie orci tincidunt adipiscing. Mauris molestie pharetra",
        subject: "Broken PR?",
        tag: "inbox",
        read: "false",
    },
    {
        id: 3,
        from: "Wylie Roberson",
        address: "mi@utnisi.edu",
        time: "2022-01-08 18:39:34",
        message:
            "Every wish you could read all this Lorem Ipsum stuff? Subornareornare lectus justo eu arcu. Morbi sit amet massa. Quisque porttitor eros nec tellus. Nunc lectus pede, ultrices",
        subject: "Learn latin in 10 days!",
        tag: "deleted",
        read: "true",
    },
    {
        id: 4,
        from: "Slack HQ",
        address: "fishbowl@slack.com",
        time: "2021-07-21 09:47:57",
        message:
            "Click here to consectetuer rhoncus. Nullam velit dui, semper et, lacinia vitae, sodales at, velit. Pellentesque ultricies dignissim lacus. Aliquam rutrum lorem ac risus. Morbi",
        subject: "Join the Fishbowl Team",
        tag: "inbox",
        read: "true",
    },
];

const getServers = async () => {

    // var settings = {
    //   "url": "http://192.168.188.128:8080/api/admin/nodes",
    //   "method": "GET",
    //   "timeout": 0,
    // };

    // $.ajax(settings).done(function (response) {
    //   console.log(response);
    //   response = response.json()
    //   response.forEach(s => {s.address = s.addr; s.status = s.stat? "true":"false"; delete s.addr; delete s.stat})
    //   return response
    // });
    try {
        const theUrl = `http://${domain}/api/admin/nodes`;

        const data = {
            method: "GET",
            // headers: {
            //   "Content-Type": "application/json",
            // },
            // body: "",
        };
        let result = await fetch(theUrl, data);
        const res = await result.json();
        res.forEach(s => { s.address = s.addr; s.status = s.stat ? "true" : "false"; delete s.addr; delete s.stat })
        return res;
    } catch (err) {
        return null;
    }
};
const getFrontEndTable = async () => {
    try {
        const theUrl = `http://${domain}/api/admin/frontendnodes`;

        const data = {
            method: "GET",
            // headers: {
            //   "Content-Type": "application/json",
            // },
            // body: "",
        };
        let result = await fetch(theUrl, data);
        const res = await result.json();
        res.forEach(s => { s.address = s.addr; delete s.addr; })
        return res;
    } catch (err) {
        return null;
    }
};

const getDetailTable = async (address) => {
    // console.log("call getDetailTable", address)
    try {
        const theUrl = `http://${domain}/api/admin/node`;
        const msg = {
            addr: address
        }
        const data = {
            method: "PUT",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(msg),
            redirect: 'follow'
        };
        let result = await fetch(theUrl, data);
        // console.log("xxx", result.body)

        const res = await result.json();
        console.log("getDetailTable", res)
        return res;
    } catch (err) {
        console.log("err", err)
        return null;
    }
};

const getRaw = async (address, row, col) => {
    try {
        const theUrl = `http://${domain}/api/admin/raw`;
        const msg = {
            addr: address,
            row: row,
            col: col
        }
        const data = {
            method: "PUT",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(msg),
            redirect: 'follow'
        };
        let result = await fetch(theUrl, data);
        const base64text = base64encode(await result.text());

        return base64text;
    } catch (err) {
        console.log("err", err)
        return "<no content>";
    }
};
const setNewPassword = async (username, newPassword) => {
    try {
        const theUrl = `http://${domain}/api/user`;
        const l = { username: username, password: newPassword }
        const data = {
            method: "put",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(l),
        };
        let result = await fetch(theUrl, data);
        const res = await result.json();
        return res;
    } catch (err) {
        return null;
    }
    return;
};
const createFolder = async (path, filename) => {
    try {
        const theUrl = `http://${domain}/api/storage${path}?type=folder&filename=${filename}`;
        const data = {
            method: "post",
            credentials: 'include'
        };
        // console.log("signup", data)
        let result = await fetch(theUrl, data);
        console.log("result", result)
        return result.status == 201;
    } catch (err) {
        return false;
    }
};
const getFolderContent = async (path) => {
    // console.log(path)
    var res = await fetch(`http://${domain}/api/storage${path}?type=folder`, {
        method: 'GET',
    })
    let result = await res.json();

    result.forEach(s => { s.isFolder = s.filename.includes(".") ? false : true;  s.name=s.filename; })
    return result;
};
const deleteFolder = async (path) => {
    let res = await fetch(`http://${domain}/api/storage/${path}`, {
        method: 'DELETE',
    })
    return res.status == 200;
};
const moveFolder = async (oldpath, newpath) => {
    let method;
    let old_elem = oldpath.split("/");
    let old_parent_path = old_elem.slice(0, old_elem.length - 2).join("/") + "/";
    let old_file_name = old_elem[old_elem.length - 2];

    let new_elem = newpath.split("/");
    let new_parent_path = new_elem.slice(0, new_elem.length - 2).join("/") + "/";
    let new_file_name = new_elem[new_elem.length - 2];

    if (old_parent_path == new_parent_path) {
        method = "rename";
    } else {
        method = "move";
    }

    let newName;
    if (method == "move") {
        newName = old_file_name
    } else {
        newName = new_file_name
    }

    new_parent_path = (new_parent_path == "/") ? "/" : ("/" + new_parent_path);

    let res = await fetch(`http://${domain}/api/storage`, {
        method: 'PUT',
        body: JSON.stringify({
            method,
            type: "folder",
            from: "/" + oldpath,
            to: new_parent_path,
            newName,
        }),
    })

    // var res = await fetch(`http://${domain}/home`, {
    //     method: 'GET',
    // })
    // return res.json()
    return { results: "this is result" };
};
const getFile = async (path) => {
    let res = await fetch(`http://${domain}/api/storage${path}`, {
        method: 'GET',
    })
    return;
    // return { results: "this is result" };
};
const deleteFile = async (path) => {
    let res = await fetch(`http://${domain}/api/storage/${path}`, {
        method: 'DELETE',
    })
    return res.status == 200;
};
const moveFile = async (oldpath, newpath) => {
    let method;
    let old_elem = oldpath.split("/");
    let old_parent_path = old_elem.slice(0, old_elem.length - 1).join("/") + "/";
    let old_file_name = old_elem[old_elem.length - 1];

    let new_elem = newpath.split("/");
    let new_parent_path = new_elem.slice(0, new_elem.length - 1).join("/") + "/";
    let new_file_name = new_elem[new_elem.length - 1];

    if (old_parent_path == new_parent_path) {
        method = "rename";
    } else {
        method = "move";
    }

    let newName;
    if (method == "move") {
        newName = old_file_name
    } else {
        newName = new_file_name
    }

    new_parent_path = (new_parent_path == "/") ? "/" : ("/" + new_parent_path);

    let res = await fetch(`http://${domain}/api/storage`, {
        method: 'PUT',
        body: JSON.stringify({
            method,
            type: "file",
            from: "/" + oldpath,
            to: new_parent_path,
            newName,
        }),
    })

    // var res = await fetch(`http://${domain}/home`, {
    //     method: 'GET',
    // })
    // return res.json()
    return { results: "this is result" };

};
const uploadFile = async (path, file) => {


    try {
        const theUrl = `http://${domain}/api/storage${path}?type=file&filename=${file.name}`;
        const data = {
            method: "post",
            headers: {
                "Content-Type": "application/json",
            },
            body: file,
        };
        let result = await fetch(theUrl, data);
        return;
    } catch (err) {
        return null;
    }


};
const signup = async (username, password) => {
    try {
        const theUrl = `http://${domain}/api/user/signup`;
        const l = { username: username, password: password }
        const data = {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(l),
        };
        let result = await fetch(theUrl, data);
        // console.log("result", result)
        // const res = await result.json();
        return;
    } catch (err) {
        return null;
    }
};
const login = async (username, password) => {
    try {
        const theUrl = `http://${domain}/api/user/login`;
        const l = { username: username, password: password }
        const data = {
            method: "post",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(l),
            credentials: 'include'
        };
        // console.log("signup", data)
        let result = await fetch(theUrl, data);
        if(result.status==200){
            return true;
        }else{
            return false;
        }
        // console.log("result", result)
        // const res = await result.headers.get('Set-Cookie');
        // console.log("headers", res)
        return true;
    } catch (err) {
        return false;
    }
    return
};
const getAllEmails = async () => {
    try {
        const theUrl = `http://${domain}/api/mail`;

        const data = {
            method: "GET",
            // headers: {
            //   "Content-Type": "application/json",
            // },
            // body: "",
        };
        let result = await fetch(theUrl, data);
        const res = await result.json();
        res.forEach((s) => {
            s.address = s.sender;
            s.from = s.sender;
            delete s.addr;
            s.message = s.content;
            s.subject = s.content.split("\n")[0];
            s.tag = "inbox";
            s.read = false;
            s.time = s.timestamp;
        });
        return res;
    } catch (err) {
        return null;
    }
};
const sendEmail = async (recipientInput, subjectInput, messageInput) => {
    try {
        const theUrl = `http://${domain}/api/mail`;
        const l = { address: recipientInput, subject: subjectInput, message: messageInput };
        const data = {
            method: "post",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(l),
            credentials: "include",
        };
        // console.log("signup", data)
        let result = await fetch(theUrl, data);
        return result;
    } catch (err) {
        return false;
    }
    return;
};
const deleteEmail = async (id) => {
    try {
        var res = await fetch(`http://${domain}/api/mail/` + id, {
            method: 'DELETE',
        });
        if (res.status != 200) {
            console.log("deleteEmail FAILED, status code: ", res.status);
            return false;
        }
        return true;
    } catch (err) {
        console.log("err: ", err);
        return false;
    }
};
const killNode = async (address) => {
    try {
        const theUrl = `http://${domain}/api/admin/kill`;
        const msg = {
            addr: address
        }
        const data = {
            method: "PUT",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(msg),
            redirect: 'follow'
        };
        let result = await fetch(theUrl, data);
        // const res = await result.json();
        // console.log("res", res)
        return result;
    } catch (err) {
        console.log("err", err)
        return null;
    }
};
const restartNode = async (address) => {
    try {
        const theUrl = `http://${domain}/api/admin/rest`;
        const msg = {
            addr: address
        }
        const data = {
            method: "PUT",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(msg),
            redirect: 'follow'
        };
        let result = await fetch(theUrl, data);
        // const res = await result.json();
        // console.log("res", res)
        return result;
    } catch (err) {
        console.log("err", err)
        return null;
    }
};

export {
    getServers,
    getDetailTable,
    getRaw,
    setNewPassword,
    createFolder,
    getFolderContent,
    deleteFolder,
    moveFolder,
    getFile,
    deleteFile,
    moveFile,
    uploadFile,
    signup,
    login,
    getAllEmails,
    sendEmail,
    deleteEmail,
    killNode,
    restartNode,
    getFrontEndTable,
};
