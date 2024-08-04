import logo from './logo.svg';
import React, { useState, useEffect } from 'react';
import Login from './component/Login';
import Options from './component/Options';
import Mailbox from './component/Mailbox';
import Storage from './component/Storage';
import MenuBar from './component/MenuBar';
import Admin from './component/Admin';


import './App.css';

function App() {
  const [status, setStatus] = useState('login');
  const [username, setUsername] = useState('');
  const submit_login = () => { 
    // check login
    setStatus('options');
  };
  const back_options = () => { 
    setStatus('options');
  };
  const click_mailbox = () => { 
    setStatus('mailbox');
  };
  const click_storage = () => { 
    setStatus('storage');
  };
  const click_admin = () => { 
    setStatus('admin');
  };
  

  return (
    // <div className="App">
    //   <header className="App-header">
    //     <img src={logo} className="App-logo" alt="logo" />
    //     <p>
    //       Edit <code>src/App.js</code> and save to reload.
    //     </p>
    //     <a
    //       className="App-link"
    //       href="https://reactjs.org"
    //       target="_blank"
    //       rel="noopener noreferrer"
    //     >
    //       Learn React
    //     </a>
    //   </header>
    // </div>
    <div className="App-container">
        <MenuBar status={status} username = {username} back_options={()=>back_options()}/>

        {status === 'login' && <Login submit_login={()=>submit_login()} setUsername={(username)=>{setUsername(username); console.log("chagne !!", username)}}/>}
        {status === 'options' && <Options click_mailbox={()=>click_mailbox()} click_storage={()=>click_storage()} click_admin={()=>click_admin()} />}
        {status === 'mailbox' && <Mailbox />}
        {status === 'storage' && <Storage />}
        {status === 'admin' && <Admin />}


    </div>
  );
}

export default App;
